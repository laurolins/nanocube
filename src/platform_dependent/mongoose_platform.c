// #include <string.h>
// #include "../thirdparty/mongoose/mongoose.h"
// #include "../thirdparty/mongoose/mongoose.h"

//
// Use mongoose as the implementation back-end of
// the nanocube request-response server of the
// platform API
//

typedef struct Server
{
	PlatformServerRequestHandler *handler;
	void *user_data;
}
Server;

internal void
mongoose_event_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	if (ev != MG_EV_RECV) {
		return;
	}

	struct mg_mgr *mgr = nc->mgr;

	Server *server = (Server*) mgr->user_data;

	pt_ServerConnection connection;
	connection.handle = nc;
	connection.user_data = server->user_data;

	struct mbuf *io = &nc->recv_mbuf;

	(*server->handler)(&connection, io->buf, io->buf + io->len);
}

// PLATFORM_SERVER_RESPOND(name) void name(PlatformServerConnection *connection, char *begin, char *end)
PLATFORM_SERVER_RESPOND(mongoose_server_respond)
{
	struct mg_connection* conn = (struct mg_connection*) connection->handle;
	mg_send(conn, begin, end-begin);
	struct mbuf *io = &conn->recv_mbuf;
	mbuf_remove(io, io->len);
	conn->flags |= MG_F_SEND_AND_CLOSE;
}

//
// Assumes a single thread will be starting a server
// no need for concurrency management here
//
PLATFORM_SERVER_START(mongoose_server_start)
{
	// @TODO(llins): configure connection to support right
	// the requested number of threads.
	Server server;
	server.handler = handler;
	server.user_data = user_data;

	struct mg_mgr mgr;
	mg_mgr_init(&mgr, &server);  // Initialize event manager object

	char buffer[32];
	Print print;
	Print_init(&print, buffer, buffer+sizeof(buffer));
	Print_u64(&print, port);
	Print_char(&print,'\0');

	// Note that many connections can be added to a single event manager
	// Connections can be created at any point, e.g. in event handler function
	mg_bind(&mgr, print.begin, mongoose_event_handler);  // Create listening connection and add it to the event manager

	for (;;) {  // Start infinite event loop
		mg_mgr_poll(&mgr, 1000);
	}

	mg_mgr_free(&mgr);
	return 0;
}



// #include "mongoose.h"  // Include Mongoose API definitions
//
// // Define an event handler function
// static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
//   struct mbuf *io = &nc->recv_mbuf;
//
//   switch (ev) {
//     case MG_EV_RECV:
//       // This event handler implements simple TCP echo server
//       mg_send(nc, io->buf, io->len);  // Echo received data back
//       mbuf_remove(io, io->len);      // Discard data from recv buffer
//       break;
//     default:
//       break;
//   }
// }
//
// int main(void) {
//   struct mg_mgr mgr;
//
//   mg_mgr_init(&mgr, NULL);  // Initialize event manager object
//
//   // Note that many connections can be added to a single event manager
//   // Connections can be created at any point, e.g. in event handler function
//   mg_bind(&mgr, "1234", ev_handler);  // Create listening connection and add it to the event manager
//
//   for (;;) {  // Start infinite event loop
//     mg_mgr_poll(&mgr, 1000);
//   }
//
//   mg_mgr_free(&mgr);
//   return 0;
// }

// internal void
// serve_request_handler(ReqRespConn *con, char *begin, char *end)
// {
//     // these are multi-threaded responses
//     platform.reqresp_respond(con, response_begin, response_end);
// }

