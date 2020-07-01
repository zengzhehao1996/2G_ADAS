#ifndef PROTO_ADAPT_H
#define PROTO_ADAPT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include <kernel.h>

#define LINDE_CAN_1275      0           // used for linde model 335-03,1275,1276
#define LINDE_CAN_1285      1           // used for linde model 1285/1286
#define LINDE_CAN_115       2           // used for linde model 115-03
#define LINDE_CAN_1191      3           // used for linde model 1191
#define LINDE_CAN_1189      4           // used for linde model 1189
#define LINDE_CAN_1169      5           // used for linde model 1169
#define LINDE_CAN_1151      6           // used for linde model 1151
#define LINDE_CAN_1158AP    7           // used for linde model 1158AP
#define LINDE_CAN_1168      8           // used for linde model 1168
#define LINDE_CAN_1193      9           // used for linde model 1193
#define LINDE_CAN_388       10          // used for linde model 388
#define LINDE_CAN_1159      11          // used for linde model 1159
#define LINDE_CAN_131       12          // used for linde model 131
#define LINDE_CAN_132       13          // used for linde model 132
#define LINDE_CAN_1158SP    14          // used for linde model 1158SP
#define LINDE_CAN_396       15          // used for linde model 396
#define LINDE_CAN_1220      16          // used for linde model 1220
#define LINDE_CAN_1158SP_GHJ    17      // used for linde model 1158sp G/H/J
#define LINDE_CAN_K011      18          // used for linde model k011
#define LINDE_CAN_35206     19          // used for linde model 352
#define LINDE_CAN_115805    20          // used for linde model 1158
#define LINDE_CAN_1183      21          // used for linde model 1183
#define LINDE_CAN_1411      22          // used for linde model 1411
#define LINDE_CAN_5195      23          // used for linde model 5195
#define LINDE_CAN_5215      24          // used for linde model 5215
#define LINDE_ADAPTATION    25          // used for adaptation 27 and 17
//abandon                   26
#define LINDE_CAN_1158V_AP_04   27      //used for linde model 1158V_AP-04

// baud rate 250k
#define LINDE_CAN_5213     51              //used for linde model 5213

#define LINDE_CAN_K5231    52            //used for linde model k5231

#define LINDE_CAN_1284     53            //used for linde E 1284

// baud rate 125k
#define LINDE_CAN_1157  101             //used for linde model 1157

    typedef struct normalProtoType
    {
        bool (*filterSet)(void);

        void (*protoParse)(uint32_t ident, uint8_t *bytes, int len);

        bool (*getCurentInfo)(void);

        bool (*getPeriodInfo)(void);
    } normalprototypet;

    enum canlib_pattern
    {
        PATTERN_0 = 0,
        PATTERN_1 = 1,
    };

    typedef void (*canTypeCallback_t) (uint8_t canType, uint8_t canPattern);

    typedef struct
    {
        uint8_t can_type;
        uint8_t can_pattern; // in enum canlib_pattern
        int32_t over_speed;
        canTypeCallback_t cantypecallback;
    }paraCanlib_t;

    bool setProtoType(paraCanlib_t para);

    bool setRecvFilter(void);

    void parseMessage(uint32_t standardId, uint8_t *recvData, uint32_t frameLength);

    void reqCurrentInfo(void);

    void reqPeriodInfo(void);

#ifdef __cplusplus
}
#endif
#endif //PROTO_ADAPT_H
