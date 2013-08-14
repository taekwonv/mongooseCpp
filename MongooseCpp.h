/**
 *	MongooseCpp
 *
 *	Cpp Wrapper of mongoose
 *
 *	@author : taekwonv@gmail.com
 */

#ifndef MONGOOSECPP_H
#define MONGOOSECPP_H

#include "../mongoose/mongoose.h"


class MgRequest
{
public:

protected:
	mg_connection *m_connection;
};


class MgResponse
{
public:

protected:
	mg_connection *m_connection;
};


class MgServer
{
public:
	MgServer(void);
	~MgServer(void);

	void Stop();
};


class MgServerImpl : public MgServer, 
	public mg_callbacks
{
public:
	MgServerImpl();
	virtual ~MgServerImpl() {}

	void context(mg_context *ctx) { m_ctx = ctx; }
	const mg_context *context() const { return m_ctx; }

protected:
	int member_begin_request(struct mg_connection *);

	mg_context *m_ctx;

private:
	static int s_begin_request(struct mg_connection *);
	static void s_end_request(const struct mg_connection *, int reply_status_code);
	static int s_log_message(const struct mg_connection *, const char *message);
	static int s_init_ssl(void *ssl_context, void *user_data);
	static int s_websocket_connect(const struct mg_connection *);
	static void s_websocket_ready(struct mg_connection *);
	static int s_websocket_data(struct mg_connection *, int bits,
							char *data, size_t data_len);
	static const char * s_open_file(const struct mg_connection *,
								const char *path, size_t *data_len);
	static void s_init_lua(struct mg_connection *, void *lua_context);
	static void s_upload(struct mg_connection *, const char *file_name);
	static int s_http_error(struct mg_connection *, int status);
};


struct MongooseCpp
{
	struct Config
	{
		unsigned short port;
	};
	static MgServer *CreateServer(Config &cfg);
};

#endif
