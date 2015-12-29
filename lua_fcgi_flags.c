//

lua_add_int_const(L, FCGI_BEGIN_REQUEST);
lua_add_int_const(L, FCGI_ABORT_REQUEST);
lua_add_int_const(L, FCGI_END_REQUEST);
lua_add_int_const(L, FCGI_PARAMS);
lua_add_int_const(L, FCGI_STDIN);
lua_add_int_const(L, FCGI_STDOUT);
lua_add_int_const(L, FCGI_STDERR);
lua_add_int_const(L, FCGI_DATA);
lua_add_int_const(L, FCGI_GET_VALUES);
lua_add_int_const(L, FCGI_GET_VALUES_RESULT);
lua_add_int_const(L, FCGI_UNKNOWN_TYPE);
lua_add_int_const(L, FCGI_NULL_REQUEST_ID);
lua_add_int_const(L, FCGI_RESPONDER);
lua_add_int_const(L, FCGI_AUTHORIZER);
lua_add_int_const(L, FCGI_FILTER);
lua_add_int_const(L, FCGI_REQUEST_COMPLETE);
lua_add_int_const(L, FCGI_CANT_MPX_CONN);
lua_add_int_const(L, FCGI_OVERLOADED);
lua_add_int_const(L, FCGI_UNKNOWN_ROLE);
