//
// Created by pborges on 7/29/16.
//

#include <Stream.h>
#include <Client.h>

#ifndef ESP_NATS_H
#define ESP_NATS_H

#define NATS_CLIENT_VERSION "0.1"
#define NATS_BUFF_SIZE 512
#define NATS_MAX_SUBS 5
#define NATS_MAX_ARGV 5

typedef void (*nats_msg_handler)(const char *);

class NATS {
private:
    nats_msg_handler *mSubs;
    const char *mHostname;

    int mSubCnt;
    Client *mClient;
    Stream *mDebug;
    IPAddress mServer;

    char *mInBuff;
    int mInOff;

    void fillBuffLine() {
        mInOff = mClient->readBytesUntil('\n', mInBuff, NATS_BUFF_SIZE);
        mInBuff[mInOff - 1] = 0x00;
    }

    void fillBuff(int n) {
        mInOff = mClient->readBytes(mInBuff, (size_t) (n + 1)); // get the \r\n
        mInBuff[mInOff - 1] = 0x00;
    }

    bool checkOk() {
        fillBuffLine();
        return strncmp(mInBuff, "+OK", 3) == 0;
    }

    void connect() {
        if (!mClient->connected()) {
            mClient->connect(mServer, 4222);
            fillBuffLine(); // READ INFO
            if (strncmp(mInBuff, "INFO", 4)) {
                mClient->stop();
                return;
            }
            mClient->printf("CONNECT {\"name\":\"%s\", \"version\":\"%s\"}\r\n", mHostname, NATS_CLIENT_VERSION);
            if (!checkOk()) {
                mClient->stop();
                return;
            }
        }
    }

public:
    NATS(const char *hostname, Client *client, IPAddress server, Stream *debug = NULL) {
        mHostname = hostname;
        mServer = server;
        mDebug = debug;
        mClient = client;
        mInBuff = (char *) malloc(NATS_BUFF_SIZE);
        mSubs = (nats_msg_handler *) calloc(NATS_MAX_SUBS, sizeof(nats_msg_handler));
    }

    bool subscribe(const char *topic, nats_msg_handler h) {
        connect();
        bool r = false;
        if (mSubCnt < NATS_MAX_SUBS) {
            mClient->printf("SUB %s %d\r\n", topic, mSubCnt);
            r = checkOk();
        }
        if (r) {
            mSubs[mSubCnt] = h;
            mSubCnt++;
        }
        return r;
    }


    // Returns INBOX ID
    bool publish(const char *topic, const char *msg) {
        if (mDebug != NULL) {
            mDebug->printf("PUB %s %d\n%s\r\n", topic, strlen(msg), msg);
        }
        mClient->printf("PUB %s %d\r\n%s\r\n", topic, strlen(msg), msg);
        return checkOk();
    }

    void loop() {
        connect();
        if (mClient->available()) {
            fillBuffLine();

            // tokenize line by space
            size_t argc = 0;
            const char *argv[NATS_MAX_ARGV] = {};
            for (int i = 0; i < NATS_MAX_ARGV; i++) {
                argv[i] = strtok((i == 0) ? mInBuff : NULL, " ");
                if (argv[i] == NULL) break;
                argc++;
            }

            if (mDebug != NULL) {
                mDebug->print("RECV ");
                mDebug->print(strlen(argv[0]));
                mDebug->print(" ");
                mDebug->println(argv[0]);
            }

            // parse message
            if (!strcmp(argv[0], "PING")) {
                mClient->println("PONG");
                return;
            }

            if (!strcmp(argv[0], "MSG")) {
                int inbox = atoi(argv[2]);
                int length = atoi(argv[3]);
                fillBuff(length);
                mSubs[inbox](mInBuff);
            }
        }
    }
};

#endif //ESP_NATS_H
