#undef  PATCH_MSG_HDLR_ARRAY
#define PATCH_MSG_HDLR_ARRAY( type )	type##_CMD_HANDLERS


//----------------------------------------------------------------------------
// REQUEST DEFINES
//----------------------------------------------------------------------------
#undef  PATCH_MSG_REQUEST_TABLE_BEGIN
#undef  PATCH_MSG_REQUEST_TABLE_DEF
#undef  PATCH_MSG_REQUEST_TABLE_END


#ifdef PATCH_MSG_REQUEST_REGISTER
#undef PATCH_MSG_REQUEST_REGISTER

#define PATCH_MSG_REQUEST_TABLE_BEGIN(t)
#define PATCH_MSG_REQUEST_TABLE_DEF(c,m,h,s,r)	{ \
	m msg; \
	MSG_STRUCT_DECL * msg_struct = msg.zz_register_msg_struct(tbl); \
	NetRegisterCommand(tbl, c, msg_struct, s, r, \
	static_cast<MFP_PRINT>(&m::Print), \
	static_cast<MFP_PUSHPOP>(&m::Push), \
	static_cast<MFP_PUSHPOP>(&m::Pop)); }
#define PATCH_MSG_REQUEST_TABLE_END(x)

#elif defined( PATCH_MSG_REQUEST_HANDLERS )
#undef		   PATCH_MSG_REQUEST_HANDLERS

#ifdef  PATCH_MSG_DEFINE_RESPONSE_HANDLERS
#define PATCH_MSG_REQUEST_TABLE_BEGIN(t)					FP_NET_RESPONSE_COMMAND_HANDLER PATCH_MSG_HDLR_ARRAY(t)[] = {
#else
#define PATCH_MSG_REQUEST_TABLE_BEGIN(t)					FP_NET_REQUEST_COMMAND_HANDLER PATCH_MSG_HDLR_ARRAY(t)[] = {
#endif
#define PATCH_MSG_REQUEST_TABLE_DEF(c,m,h,s,r)				h,
#define PATCH_MSG_REQUEST_TABLE_END(x)						NULL};

#else

#define PATCH_MSG_REQUEST_TABLE_BEGIN(t)					enum t##_REQUEST_COMMANDS {
#define PATCH_MSG_REQUEST_TABLE_DEF(c,m,h,s,r)				c,
#define PATCH_MSG_REQUEST_TABLE_END(x)						x };

#endif



//----------------------------------------------------------------------------
// RESPONSE DEFINES
//----------------------------------------------------------------------------
#undef  PATCH_MSG_RESPONSE_TABLE_BEGIN
#undef  PATCH_MSG_RESPONSE_TABLE_DEF
#undef  PATCH_MSG_RESPONSE_TABLE_END


//----------------------------------------------------------------------------
//	register the message with the command table
//----------------------------------------------------------------------------
#ifdef PATCH_MSG_RESPONSE_REGISTER
#undef PATCH_MSG_RESPONSE_REGISTER

#define PATCH_MSG_RESPONSE_TABLE_BEGIN(t)
#define PATCH_MSG_RESPONSE_TABLE_DEF(c,m,s,r)	{ \
	m msg; \
	MSG_STRUCT_DECL * msg_struct = msg.zz_register_msg_struct(tbl); \
	NetRegisterCommand(tbl, c, msg_struct, s, r, \
	static_cast<MFP_PRINT>(&m::Print), \
	static_cast<MFP_PUSHPOP>(&m::Push), \
	static_cast<MFP_PUSHPOP>(&m::Pop)); }
#define PATCH_MSG_RESPONSE_TABLE_END(x)

//----------------------------------------------------------------------------
// define command enum
//----------------------------------------------------------------------------
#else

#define PATCH_MSG_RESPONSE_TABLE_BEGIN(t)					enum t##_RESPONSE_COMMANDS {
#define PATCH_MSG_RESPONSE_TABLE_DEF(c,m,s,r)				c,
#define PATCH_MSG_RESPONSE_TABLE_END(x)						x };

#endif	//	response message defines
