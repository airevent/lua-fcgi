// fcgi

#include "lua_fcgi.h"

LUAMOD_API int luaopen_fcgi( lua_State *L ) {
    luaL_newlib(L, __index);

    lua_newtable(L);
        #include "lua_fcgi_flags.c"
    lua_setfield(L, -2, "f");

    return 1;
}

//

// arg#1 - table with packets(tables)
// it will return concat'd binary string or nil, es, en
static int lua_fcgi_pack( lua_State *L ) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);

    lua_getfield(L, 1, "id");
    lua_getfield(L, 1, "type");

    int id = lua_tointeger(L, 2);
    int type = lua_tointeger(L, 3);

    size_t content_len;
    const char *str;

    luaL_Buffer B;
    luaL_buffinit(L, &B);

    FCGI_EndRequestBody er_body;

    switch ( type ) {
        case FCGI_BEGIN_REQUEST:
            break;
        case FCGI_ABORT_REQUEST:
            break;
        case FCGI_END_REQUEST:
            //lua_getfield(L, 1, "app_status");
            lua_getfield(L, 1, "protocol_status");

            content_len = sizeof(er_body);

            lua_fcgi_addheader(&B, id, type, content_len);

            er_body.appStatusB3 = 0;
            er_body.appStatusB2 = 0;
            er_body.appStatusB1 = 0;
            er_body.appStatusB0 = 0;
            er_body.protocolStatus = (unsigned char)lua_tointeger(L, -1);

            luaL_addlstring(&B, (char *)&er_body, sizeof(er_body));

            lua_fcgi_addpad(&B, content_len);
            break;
        case FCGI_PARAMS:
            break;
        case FCGI_STDIN:
        case FCGI_STDOUT:
        case FCGI_STDERR:
        case FCGI_DATA:
            lua_getfield(L, 1, "body");

            if ( lua_type(L, -1) != LUA_TSTRING ) {
                content_len = 0;

                lua_fcgi_addheader(&B, id, type, content_len);

                lua_fcgi_addpad(&B, content_len);
            } else {
                str = luaL_tolstring(L, -1, &content_len);

                lua_fcgi_addheader(&B, id, type, content_len);

                luaL_addlstring(&B, str, content_len);

                lua_fcgi_addpad(&B, content_len);
            }
            break;
        case FCGI_GET_VALUES:
            break;
        case FCGI_GET_VALUES_RESULT:
            break;
        default: // FCGI_UNKNOWN_TYPE
            break;
    }

    luaL_pushresult(&B);
    return 1;
}

// arg#1 - str
// will parse str and divide records
// will return records and the remaining unused bytes count
static int lua_fcgi_unpack( lua_State *L ) {
    size_t len;
    size_t pos = 0;
    const char *str = lua_tolstring(L, 1, &len);

    int records_found = 0;

    FCGI_Header header;
    FCGI_BeginRequestBody br_body;
    FCGI_EndRequestBody er_body;
    FCGI_UnknownTypeBody ut_body;

    int content_len;
    int key_len;
    int val_len;
    size_t kv_offset;

    while ( pos + FCGI_HEADER_LEN <= len ) {
        memcpy(&header, &str[pos], FCGI_HEADER_LEN);
        pos += FCGI_HEADER_LEN;

        content_len = (header.contentLengthB1 << 8) + header.contentLengthB0;

        if ( pos + content_len + header.paddingLength > len ) {
            // not enough bytes in input str
            break;
        } else {
            if ( header.type < FCGI_MINTYPE || header.type > FCGI_MAXTYPE ) {
                header.type = FCGI_UNKNOWN_TYPE;
            }

            if ( records_found==0 ) {
                lua_newtable(L); // table for all found records - lazy init
            }

            records_found++;

            lua_pushinteger(L, records_found);
            lua_newtable(L); // for current found record

            lua_pushinteger(L, (header.requestIdB1 << 8) + header.requestIdB0);
            lua_setfield(L, -2, "id");

            lua_pushinteger(L, header.type);
            lua_setfield(L, -2, "type");

            switch ( header.type ) {
                case FCGI_BEGIN_REQUEST:
                    if ( content_len != sizeof(FCGI_BeginRequestBody) ) {
                        break;
                    }

                    memcpy(&br_body, &str[pos], content_len);

                    lua_pushinteger(L, (br_body.roleB1 << 8) + br_body.roleB0);
                    lua_setfield(L, -2, "role");

                    lua_pushboolean(L, (br_body.flags & FCGI_KEEP_CONN)==1);
                    lua_setfield(L, -2, "keepalive");
                    break;
                case FCGI_ABORT_REQUEST: // just id and type, no fields
                    break;
                case FCGI_END_REQUEST:
                    if ( content_len != sizeof(FCGI_EndRequestBody) ) {
                        break;
                    }

                    memcpy(&er_body, &str[pos], content_len);

                    lua_pushinteger(L,
                        ((er_body.appStatusB3 & 0x7f) << 24)
                        + (er_body.appStatusB2 << 16)
                        + (er_body.appStatusB1 << 8)
                        + er_body.appStatusB0);
                    lua_setfield(L, -2, "app_status");

                    lua_pushinteger(L, er_body.protocolStatus);
                    lua_setfield(L, -2, "protocol_status");
                    break;
                case FCGI_PARAMS:
                case FCGI_GET_VALUES:
                case FCGI_GET_VALUES_RESULT:
                    if ( content_len == 0 ) {
                        break;
                    }

                    lua_newtable(L); // params

                    kv_offset = 0;

                    while ( kv_offset + 2 <= content_len ) { // 2 is minimal length (1 byte key len + 1 byte val len)
                        // key

                        if ( str[pos + kv_offset] >> 7 == 0 ) { // key length is 1 byte
                            key_len = str[pos + kv_offset];
                            kv_offset++;
                        } else { // key length is 4 bytes
                            if ( kv_offset + 4 > content_len ) {
                                break;
                            }

                            key_len =
                                ((str[pos + kv_offset] & 0x7f) << 24)
                                + (str[pos + kv_offset + 1] << 16)
                                + (str[pos + kv_offset + 2] << 8)
                                + str[pos + kv_offset + 3];
                            kv_offset += 4;
                        }

                        // value

                        if ( str[pos + kv_offset] >> 7 == 0 ) { // val length is 1 byte
                            val_len = str[pos + kv_offset];
                            kv_offset++;
                        } else { // val length is 4 bytes
                            if ( kv_offset + 4 > content_len ) {
                                break;
                            }

                            val_len =
                                ((str[pos + kv_offset] & 0x7f) << 24)
                                + (str[pos + kv_offset + 1] << 16)
                                + (str[pos + kv_offset + 2] << 8)
                                + str[pos + kv_offset + 3];
                            kv_offset += 4;
                        }

                        //

                        if ( kv_offset + key_len + val_len <= content_len ) {
                            lua_pushlstring(L, &str[pos + kv_offset], key_len);
                            lua_pushlstring(L, &str[pos + kv_offset + key_len], val_len);
                            lua_rawset(L, -3); // param
                            kv_offset += (key_len + val_len);
                        }
                    }

                    lua_setfield(L, -2, "params");
                    break;
                case FCGI_STDIN:
                case FCGI_STDOUT:
                case FCGI_STDERR:
                case FCGI_DATA:
                    if ( content_len == 0 ) {
                        break;
                    }
                    lua_pushlstring(L, &str[pos], content_len);
                    lua_setfield(L, -2, "body");
                    break;
                default: // FCGI_UNKNOWN_TYPE
                    if ( content_len != sizeof(FCGI_UnknownTypeBody) ) {
                        break;
                    }

                    memcpy(&ut_body, &str[pos], content_len);

                    lua_pushinteger(L, ut_body.type);
                    lua_setfield(L, -2, "bad_type");
                    break;
            }

            lua_rawset(L, -3); // for current found record

            pos += (content_len + header.paddingLength);
        }
    }

    if ( records_found > 0 ) {
        lua_pushinteger(L, len-pos);
        return 2;
    } else {
        lua_pushnil(L);
        lua_pushinteger(L, len-pos);
        return 2;
    }
}

//

static void lua_fcgi_addheader( luaL_Buffer *B, int id, int type, size_t content_len ) {
    FCGI_Header header = {0};

    header.version = FCGI_VERSION_1;
    header.type = (unsigned char)type;
    header.requestIdB1 = (unsigned char)((id >> 8) & 0xff);
    header.requestIdB0 = (unsigned char)(id & 0xff);
    header.contentLengthB1 = (unsigned char)((content_len >> 8) & 0xff);
    header.contentLengthB0 = (unsigned char)(content_len & 0xff);
    header.paddingLength = ((content_len + 7) & ~7) - content_len;

    luaL_addlstring(B, (char *)&header, sizeof(header));
}

static void lua_fcgi_addpad( luaL_Buffer *B, size_t content_len ) {
    size_t paddingLength = ((content_len + 7) & ~7) - content_len;

    if ( paddingLength > 0 ) {
        luaL_addlstring(B, "MARSGPL", paddingLength);
    }
}
