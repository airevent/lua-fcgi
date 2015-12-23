// fcgi

#include "lua_fcgi.h"

LUAMOD_API int luaopen_fcgi( lua_State *L ) {
    luaL_newlib(L, __index);
    return 1;
}

//

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
                case FCGI_PARAMS:
                    if ( content_len == 0 ) {
                        break;
                    }

                    lua_newtable(L); // params

                    kv_offset = 0;

                    while ( kv_offset + 2 <= content_len ) { // 2 is minimal length (1 byle key len + 1 byte val len)
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
                    if ( content_len == 0 ) {
                        break;
                    }
                    lua_pushlstring(L, &str[pos], content_len);
                    lua_setfield(L, -2, "body");
                    break;
                case FCGI_STDOUT:
                    if ( content_len == 0 ) {
                        break;
                    }
                    lua_pushlstring(L, &str[pos], content_len);
                    lua_setfield(L, -2, "body");
                    break;
                case FCGI_STDERR:
                    if ( content_len == 0 ) {
                        break;
                    }
                    lua_pushlstring(L, &str[pos], content_len);
                    lua_setfield(L, -2, "body");
                    break;
                case FCGI_DATA:
                    if ( content_len == 0 ) {
                        break;
                    }
                    lua_pushlstring(L, &str[pos], content_len);
                    lua_setfield(L, -2, "body");
                    break;
                case FCGI_ABORT_REQUEST:
printf("TODO: FCGI_ABORT_REQUEST\n");
                    break;
                case FCGI_GET_VALUES:
printf("TODO: FCGI_GET_VALUES\n");
                    break;
                case FCGI_GET_VALUES_RESULT:
printf("TODO: FCGI_GET_VALUES_RESULT\n");
                    break;
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
                default: // FCGI_UNKNOWN_TYPE
                    if ( content_len != sizeof(FCGI_UnknownTypeBody) ) {
                        break;
                    }

                    memcpy(&ut_body, &str[pos], content_len);

                    lua_pushinteger(L, ut_body.type);
                    lua_setfield(L, -2, "un_type");

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

// arg#1 - table with packets(tables)
// it will return concat'd binary string or nil, es, en
static int lua_fcgi_pack( lua_State *L ) {
    lua_fail(L, "TODO", 0);
}
