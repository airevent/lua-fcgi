// fcgi

#include "lua_pd.h"

//

#define FCGI_HEADER_LEN 8

#define FCGI_VERSION_1 1

#define FCGI_BEGIN_REQUEST 1
#define FCGI_ABORT_REQUEST 2
#define FCGI_END_REQUEST 3
#define FCGI_PARAMS 4
#define FCGI_STDIN 5
#define FCGI_STDOUT 6
#define FCGI_STDERR 7
#define FCGI_DATA 8
#define FCGI_GET_VALUES 9
#define FCGI_GET_VALUES_RESULT 10
#define FCGI_UNKNOWN_TYPE 11

#define FCGI_MINTYPE (FCGI_BEGIN_REQUEST)
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

#define FCGI_NULL_REQUEST_ID 0

#define FCGI_KEEP_CONN 1

#define FCGI_RESPONDER 1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER 3

#define FCGI_MINROLE (FCGI_RESPONDER)
#define FCGI_MAXROLE (FCGI_FILTER)

#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN 1
#define FCGI_OVERLOADED 2
#define FCGI_UNKNOWN_ROLE 3

#define FCGI_MAX_CONNS "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

//

typedef struct FCGI_Header {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} FCGI_Header;

typedef struct FCGI_BeginRequestBody {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} FCGI_BeginRequestBody;

typedef struct FCGI_EndRequestBody {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} FCGI_EndRequestBody;

typedef struct FCGI_UnknownTypeBody {
    unsigned char type;
    unsigned char reserved[7];
} FCGI_UnknownTypeBody;

//

LUAMOD_API int luaopen_fcgi( lua_State *L );

static int lua_fcgi_unpack( lua_State *L );
static int lua_fcgi_pack( lua_State *L );

//

static const luaL_Reg __index[] = {
    {"unpack", lua_fcgi_unpack},
    {"pack", lua_fcgi_pack},
    {NULL, NULL}
};
