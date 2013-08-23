/**
 *	MongooseCpp
 *
 *	Cpp Wrapper of mongoose
 *
 *	@author		taekwonv@gmail.com
 *	@version	0.130822
 */

#ifndef MONGOOSECPP_H
#define MONGOOSECPP_H

#include "../mongoose/mongoose.h"
#include <functional>
#include <memory>


/**
 *	MgRequest
 *
 *	The object of this class is created internally and returned from MgServer::IListener::OnRequest(). It represents an in-progress request from the client side.
 *
 */

class MgRequest
{
public:
	int read(char *buf, size_t len);
	std::string requestMethod() const;	// "GET", "POST", etc
	std::string uri() const;			// URL-decoded URI
	std::string httpVersion() const;	// E.g. "1.0", "1.1"
	std::string queryString() const;	// URL part after '?', not including '?', or NULL
	std::string remoteUser() const;		// Authenticated user, or NULL if no auth used
	long remoteIP() const;				// Client's IP address
	int remotePort() const;				// Client's port
	int is_ssl() const;					// 1 if SSL-ed, 0 if not

	void onEndRequest(std::function<void(int reply_status_code)> handler) { m_onEndRequest = handler; }
	void onLogMessage(std::function<int(const char *message)> handler) { m_onLogMessage = handler; }
	void onWebSocketConnect(std::function<int()> handler) { m_onWebSocketConnect = handler; }
	void onWebSocketReady(std::function<void()> handler) { m_onWebSocketReady = handler; }
	void onWebSocketData(std::function<int(int bits, char *data, size_t data_len)> handler) { m_onWebSocketData = handler; }
	void onOpenFile(std::function<const char *(const char *path, size_t *data_len)> handler) { m_onOpenFile = handler; }
	void onInitLua(std::function<void(void *lua_context)> handler) { m_onInitLua = handler; }
	void onUpload(std::function<void(const char *file_name)> handler) { m_onUpload = handler; }
	void onHttpError(std::function<int(int status)> handler) { m_onHttpError = handler; }
	
	MgRequest(mg_connection *con);
	~MgRequest();

protected:
	mg_connection *m_connection;
	
	friend class MgServerImpl;
	std::function<void(int reply_status_code)> m_onEndRequest;
	std::function<int(const char *message)> m_onLogMessage;
	std::function<int()> m_onWebSocketConnect;
	std::function<void()> m_onWebSocketReady;
	std::function<int(int bits, char *data, size_t data_len)> m_onWebSocketData;
	std::function<const char *(const char *path, size_t *data_len)> m_onOpenFile;
	std::function<void(void *lua_context)> m_onInitLua;
	std::function<void(const char *file_name)> m_onUpload;
	std::function<int(int status)> m_onHttpError;

private:
	MgRequest() {}
};


/**
 *	MgResponse
 *
 *	The object of this class is created internally and returned from MgServer::IListener::OnRequest(). All you want to respond to the client may be implemented by using this object.
 *
 */

class MgResponse
{
public:	
	void setCache(bool b) { m_cache = b; }
	void setTimeout(unsigned short sec) { m_timeoutSec = sec; }
	void setStatusCode(int statusCode) { m_statusCode = statusCode; }
	void setHeader(int statusCode, const char *httpVersion, const char *content_type, unsigned long content_length, const char *transfer_encoding);
	void writeHeader(int statusCode, const char *httpVersion, const char *content_type, unsigned long content_length, const char *transfer_encoding);
	void write(const std::string &content);
	void end();

	MgResponse(struct mg_connection *);
	~MgResponse();

protected:
	MgResponse() {}
	mg_connection *m_connection;
	bool m_cache;
	int	m_statusCode;
	unsigned short m_timeoutSec;
	std::string m_httpVersion;
	std::string m_contentType;
	unsigned long m_contentLength;
	std::string m_transferEncoding;
};


/**
 *	MgServer
 *
 *	The object of this class is created internally and returned from MongooseCpp::CreateServer(). It represent a HTTP(S) server instance.
 *
 */

class MgServer
{
public:
	MgServer(void);
	~MgServer(void);

	void stop();
	void listen(std::function<int(MgRequest *req, MgResponse *res)> onNewRequest) { m_listener = onNewRequest; }

protected:
	std::function<int(MgRequest *req, MgResponse *res)> m_listener;
};


/**
 *	MongooseCpp
 *
 *	The entry point of MongooseCpp.
 *
 */

struct MongooseCpp
{
	struct ServerConfig
	{
		unsigned short port;
	};
	static MgServer *createServer(ServerConfig &cfg);

	struct RequestInfo
	{
		std::string	destAddr;		
		std::string method; // POST or GET
		std::string httpVersion; // ex) HTTP/1.1
		std::string uri;
		std::string data;
		unsigned short port;
		bool usessl;
	};
	static MgRequest *request(RequestInfo &);
};


/**
 *	MgServerImpl
 *
 *	The implementation of MgServer class (Bridge Design Pattern).
 *
 */

class MgServerImpl : public MgServer, 
	public mg_callbacks
{
public:
	MgServerImpl();
	virtual ~MgServerImpl() {}

	void context(mg_context *ctx) { m_ctx = ctx; }
	const mg_context *context() const { return m_ctx; }

	// callbacks
	int member_begin_request(struct mg_connection *);
	void member_end_request(const struct mg_connection *, int reply_status_code);
	int member_log_message(const struct mg_connection *, const char *message);	
	int member_websocket_connect(const struct mg_connection *);
	void member_websocket_ready(struct mg_connection *);
	int member_websocket_data(struct mg_connection *, int bits, char *data, size_t data_len);
	const char * member_open_file(const struct mg_connection *, const char *path, size_t *data_len);
	void member_init_lua(struct mg_connection *, void *lua_context);
	void member_upload(struct mg_connection *, const char *file_name);
	int member_http_error(struct mg_connection *, int status);
	static int s_init_ssl(void *ssl_context, void *user_data);

protected:
	mg_context *m_ctx;
};
#endif
