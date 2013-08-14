/**
 *                                                  
 *  _____                             _____         
 * |     |___ ___ ___ ___ ___ ___ ___|     |___ ___ 
 * | | | | . |   | . | . | . |_ -| -_|   --| . | . |
 * |_|_|_|___|_|_|_  |___|___|___|___|_____|  _|  _|
 *               |___|                     |_| |_|  
 */

#include "MongooseCpp.h"



MgServer::MgServer(void)
{
}


MgServer::~MgServer(void)
{
}


MgServerImpl::MgServerImpl()
	: m_ctx(NULL)
{
	this->begin_request = &MgServerImpl::s_begin_request;
}

int MgServerImpl::member_begin_request(struct mg_connection *)
{
	return 0;
}

int MgServerImpl::s_begin_request(struct mg_connection *con)
{
	mg_request_info *info = mg_get_request_info(con);
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	return server->member_begin_request(con);
}

void MgServerImpl::s_end_request(const struct mg_connection *con, int reply_status_code)
{	
}

int MgServerImpl::s_log_message(const struct mg_connection *, const char *message){ return 0; }
int MgServerImpl::s_init_ssl(void *ssl_context, void *user_data){ return 0; }
int MgServerImpl::s_websocket_connect(const struct mg_connection *){ return 0; }
void MgServerImpl::s_websocket_ready(struct mg_connection *){}
int MgServerImpl::s_websocket_data(struct mg_connection *, int bits,
				char *data, size_t data_len){ return 0; }
const char * MgServerImpl::s_open_file(const struct mg_connection *,
					const char *path, size_t *data_len){ return NULL; }
void MgServerImpl::s_init_lua(struct mg_connection *, void *lua_context){}
void MgServerImpl::s_upload(struct mg_connection *, const char *file_name){}
int MgServerImpl::s_http_error(struct mg_connection *, int status){ return 0; }


MgServer *MongooseCpp::CreateServer(MongooseCpp::Config &cfg)
{
	MgServerImpl *server = new MgServerImpl;
	
	const char *options[] = {
		"document_root", "/var/www",
		"listening_ports", "80,443",
		NULL
	};

	mg_context *ctx = mg_start(server, server, options);
	if (NULL == ctx)
	{
		delete server;
		return NULL;
	}

	server->context(ctx);

	return server;
}
