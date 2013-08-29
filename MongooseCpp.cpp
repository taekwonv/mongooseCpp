/**
 *                                                  
 *  _____                             _____         
 * |     |___ ___ ___ ___ ___ ___ ___|     |___ ___ 
 * | | | | . |   | . | . | . |_ -| -_|   --| . | . |
 * |_|_|_|___|_|_|_  |___|___|___|___|_____|  _|  _|
 *               |___|                     |_| |_|  
 */

#include "MongooseCpp.h"
#include <unordered_map>

using namespace std;


#ifdef __linux__
#include <linux/spinlock.h>

class SpinLock
{
	spinlock_t m_sl;

public:
	SpinLock(void) { spin_lock_init(&m_sl); }
	~SpinLock(void){}

	void lock()	{ spin_lock(&m_sl); }
	void unlock() {	spin_unlock(&m_sl);	}
};


#elif defined _WIN32 || defined _WIN64
	#if _WIN32_WINN < T0x0403
		#error for InitializeCriticalSectionAndSpinCount you must define this in project property.
	#endif

#include <windows.h>

#define SPINLOCK_SPINCOUNT	4000


class SpinLock
{
	CRITICAL_SECTION m_cs;

public:
	SpinLock() { InitializeCriticalSectionAndSpinCount(&m_cs, SPINLOCK_SPINCOUNT); }
	~SpinLock()	{ DeleteCriticalSection(&m_cs); }
	void lock() { EnterCriticalSection(&m_cs); }
	void unlock() { LeaveCriticalSection(&m_cs); }
};

#else
#error "NOT SUPPORTED TARGET"
#endif


template <class KeyT, class ValT>
class ObjectMap
{
	SpinLock m_lock;
	unordered_map<KeyT, ValT> m_map;	
	template <class LockT> struct LocalLock_ { LocalLock_(LockT *l) : m_l(l) { m_l->lock(); } ~LocalLock_() { m_l->unlock(); } LockT *m_l; };

public:
	void add(KeyT k, ValT v) { LocalLock_<SpinLock> ll(&m_lock); m_map[k] = v; }
	bool find(KeyT k, ValT &v/*out*/)
	{
		LocalLock_<SpinLock> ll(&m_lock);
		auto it = m_map.find(k);
		if (it == m_map.end())	return false;
		v = it->second;
		return true;
	}
	void remove(KeyT k) { LocalLock_<SpinLock> ll(&m_lock); m_map.erase(k); }
};

static ObjectMap<const mg_connection *, MgRequest *>	s_objMapMgRequest;
static ObjectMap<const mg_connection *, MgResponse *>	s_objMapMgResponse;

MgRequest *FindMgRequest(const mg_connection *con)
{
	MgRequest *req = NULL;
	s_objMapMgRequest.find(con, req);
	return req;
}

MgResponse *FindMgResponse(const mg_connection *con)
{
	MgResponse *res = NULL;
	s_objMapMgResponse.find(con, res);
	return res;
}


MgServer::MgServer(void) : m_listener(NULL) {}

MgServer::~MgServer(void) {}

MgServer *MongooseCpp::createServer(MongooseCpp::ServerConfig &cfg, const char **mongooseOption)
{
#ifndef NO_SSL
	// If compiles with SSL, make sure cfg has ssl_certificate.
	if (cfg.ssl_certificate.length() < 1)
		return NULL;
#endif
	MgServerImpl *server = new MgServerImpl();
		
	#define MAX_NUM_OPTION	100
	const char *options[MAX_NUM_OPTION];

	if (NULL == mongooseOption)
	{
		int i = 0;
		cfg.cgi_pattern.length() > 0 ? options[i++] = "cgi_pattern", options[i++] = cfg.cgi_pattern.c_str() : __noop;
		cfg.cgi_environment.length() > 0 ? options[i++]  = "cgi_environment", options[i++] =  cfg.cgi_environment.c_str() : __noop;
		cfg.put_delete_auth_file.length() > 0 ? options[i++]  = "put_delete_auth_file", options[i++] =  cfg.put_delete_auth_file.c_str() : __noop;
		cfg.cgi_interpreter.length() > 0 ? options[i++]  = "cgi_interpreter", options[i++] =  cfg.cgi_interpreter.c_str() : __noop;
		cfg.protect_uri.length() > 0 ? options[i++]  = "protect_uri", options[i++] =  cfg.protect_uri.c_str() : __noop;
		cfg.authentication_domain.length() > 0 ? options[i++]  = "authentication_domain", options[i++] =  cfg.authentication_domain.c_str() : __noop;
		cfg.ssi_pattern.length() > 0 ? options[i++]  = "ssi_pattern", options[i++] =  cfg.ssi_pattern.c_str() : __noop;
		cfg.throttle.length() > 0 ? options[i++]  = "throttle", options[i++] =  cfg.throttle.c_str() : __noop;
		cfg.access_log_file.length() > 0 ? options[i++]  = "access_log_file", options[i++] =  cfg.access_log_file.c_str() : __noop;
		cfg.enable_directory_listing.length() > 0 ? options[i++]  = "enable_directory_listing", options[i++] =  cfg.enable_directory_listing.c_str() : __noop;
		cfg.error_log_file.length() > 0 ? options[i++]  = "error_log_file", options[i++] =  cfg.error_log_file.c_str() : __noop;
		cfg.global_auth_file.length() > 0 ? options[i++]  = "global_auth_file", options[i++] =  cfg.global_auth_file.c_str() : __noop;
		cfg.index_files.length() > 0 ? options[i++]  = "index_files", options[i++] =  cfg.index_files.c_str() : __noop;
		cfg.enable_keep_alive.length() > 0 ? options[i++]  = "enable_keep_alive", options[i++] =  cfg.enable_keep_alive.c_str() : __noop;
		cfg.access_control_list.length() > 0 ? options[i++]  = "access_control_list", options[i++] =  cfg.access_control_list.c_str() : __noop;
		cfg.extra_mime_types.length() > 0 ? options[i++]  = "extra_mime_types", options[i++] =  cfg.extra_mime_types.c_str() : __noop;
		cfg.listening_ports.length() > 0 ? options[i++]  = "listening_ports", options[i++] =  cfg.listening_ports.c_str() : __noop;
		cfg.document_root.length() > 0 ? options[i++]  = "document_root", options[i++] =  cfg.document_root.c_str() : __noop;
		cfg.ssl_certificate.length() > 0 ? options[i++]  = "ssl_certificate", options[i++] =  cfg.ssl_certificate.c_str() : __noop;
		cfg.num_threads.length() > 0 ? options[i++]  = "num_threads", options[i++] =  cfg.num_threads.c_str() : __noop;
		cfg.run_as_user.length() > 0 ? options[i++]  = "run_as_user", options[i++] =  cfg.run_as_user.c_str() : __noop;
		cfg.url_rewrite_patterns.length() > 0 ? options[i++]  = "url_rewrite_patterns", options[i++] =  cfg.url_rewrite_patterns.c_str() : __noop;
		cfg.hide_files_patterns.length() > 0 ? options[i++]  = "hide_files_patterns", options[i++] =  cfg.hide_files_patterns.c_str() : __noop;
		cfg.request_timeout_ms.length() > 0 ? options[i++]  = "request_timeout_ms", options[i++] =  cfg.request_timeout_ms.c_str() : __noop;
		options[i] = NULL;
	}

	mg_context *ctx = mg_start(server, server, mongooseOption ? mongooseOption : options);
	if (NULL == ctx)
	{
		delete server;
		return NULL;
	}

	server->context(ctx);

	return server;
}

MgRequest *MongooseCpp::request(RequestInfo &info)
{
	char errbuf[1024];
	mg_connection *con = mg_download(info.destAddr.c_str(), info.port, info.usessl ? 1 : 0, errbuf, sizeof(errbuf), 
		"%s %s %s\r\n"
		"Host: %s\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s",
		info.method.c_str(), info.uri.c_str(), info.httpVersion.c_str(),
		info.destAddr.c_str(),
		info.data.length(),
		info.data.c_str());

	if (NULL == con) return NULL;

	return new MgRequest(con);
}


/**
 *  _____     _____                     _____           _ 
 * |     |___|   __|___ ___ _ _ ___ ___|     |_____ ___| |
 * | | | | . |__   | -_|  _| | | -_|  _|-   -|     | . | |
 * |_|_|_|_  |_____|___|_|  \_/|___|_| |_____|_|_|_|  _|_|
 *       |___|                                     |_|    
 */                                                       


static int static_begin_request(struct mg_connection *con) 
{
	mg_request_info *info = mg_get_request_info(con);
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return 0; 
	return server->member_begin_request(con);
}

static void static_end_request(const struct mg_connection *con, int reply_status_code) 
{
	mg_request_info *info = mg_get_request_info(const_cast<struct mg_connection *>(con));
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return; 
	return server->member_end_request(con, reply_status_code);
}

static int static_log_message(const struct mg_connection *con, const char *message) 
{
	mg_request_info *info = mg_get_request_info(const_cast<struct mg_connection *>(con));
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);	
	if (NULL == server) return 0; 
	return server->member_log_message(con, message);
}	

static int static_websocket_connect(const struct mg_connection *con) 
{ 
	mg_request_info *info = mg_get_request_info(const_cast<struct mg_connection *>(con));
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return 0; 
	return server->member_websocket_connect(con);
}

static void static_websocket_ready(struct mg_connection *con) 
{
	mg_request_info *info = mg_get_request_info(con);
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return; 
	return server->member_websocket_ready(con);
}

static int static_websocket_data(struct mg_connection *con, int bits, char *data, size_t data_len) 
{
	mg_request_info *info = mg_get_request_info(con);
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return 0; 
	return server->member_websocket_data(con, bits, data, data_len);
}

static const char * static_open_file(const struct mg_connection *con, const char *path, size_t *data_len) 
{
	mg_request_info *info = mg_get_request_info(const_cast<struct mg_connection *>(con));
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return NULL; 
	return server->member_open_file(con, path, data_len);
}

static void static_init_lua(struct mg_connection *con, void *lua_context) 
{
	mg_request_info *info = mg_get_request_info(con);
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return;
	return server->member_init_lua(con, lua_context);
}

static void static_upload(struct mg_connection *con, const char *file_name) 
{
	mg_request_info *info = mg_get_request_info(con);
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return; 
	return server->member_upload(con, file_name);
}

static int static_http_error(struct mg_connection *con, int status) 
{
	mg_request_info *info = mg_get_request_info(con);
	MgServerImpl *server = reinterpret_cast<MgServerImpl *>(info->user_data);
	if (NULL == server) return 0; 
	return server->member_http_error(con, status);
}

MgServerImpl::MgServerImpl()
	: m_ctx(NULL)
{
	// map callbacks to member funcs		
	this->begin_request = static_begin_request;
	this->end_request = static_end_request;
	this->log_message = static_log_message;
#ifdef NO_SSL
	this->init_ssl = NULL;
#else
	this->init_ssl = s_init_ssl;
#endif
	this->websocket_connect = static_websocket_connect;
	this->websocket_ready = static_websocket_ready;
	this->websocket_data = static_websocket_data;
	this->open_file = static_open_file;
	this->init_lua = static_init_lua;
	this->upload = static_upload;
	this->http_error = static_http_error;
}

int MgServerImpl::member_begin_request(struct mg_connection *con)
{
	if (m_listener)
	{
		mg_request_info *info = mg_get_request_info(con);
		MgResponse *res = new MgResponse(con);
		MgRequest *req = new MgRequest(con);
		return m_listener(req, res);
	}
	return 0;
}

void MgServerImpl::member_end_request(const struct mg_connection *con, int reply_status_code)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		req->m_onEndRequest(reply_status_code);
}

int MgServerImpl::member_log_message(const struct mg_connection *con, const char *message)	
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		return req->m_onLogMessage(message);
	return 0;
}

int MgServerImpl::member_websocket_connect(const struct mg_connection *con)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		return req->m_onWebSocketConnect();
	return 0;
}

void MgServerImpl::member_websocket_ready(struct mg_connection *con)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		req->m_onWebSocketReady();
}

int MgServerImpl::member_websocket_data(struct mg_connection *con, int bits, char *data, size_t data_len)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		return req->m_onWebSocketData(bits, data, data_len);
	return 0;
}

const char *MgServerImpl::member_open_file(const struct mg_connection *con, const char *path, size_t *data_len)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		return req->m_onOpenFile(path, data_len);
	return NULL;
}

void MgServerImpl::member_init_lua(struct mg_connection *con, void *lua_context)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		req->m_onInitLua(lua_context);
}

void MgServerImpl::member_upload(struct mg_connection *con, const char *file_name)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		req->m_onUpload(file_name);
}

int MgServerImpl::member_http_error(struct mg_connection *con, int status)
{
	MgRequest *req = FindMgRequest(con);
	if (req)
		return req->m_onHttpError(status);
	return 0;
}

int MgServerImpl::s_init_ssl(void *ssl_context, void *user_data)
{
	// TODO

	// If it returns non-NULL, that means that this has
	// set up certificate itself. In this case, skip sertificate setting.
	return 0;
}


/**
 *  _____     _____                             
 * |     |___| __  |___ ___ ___ ___ ___ ___ ___ 
 * | | | | . |    -| -_|_ -| . | . |   |_ -| -_|
 * |_|_|_|_  |__|__|___|___|  _|___|_|_|___|___|
 *       |___|             |_|                  
 */
MgResponse::MgResponse(struct mg_connection *con) 
	: m_connection(con)
{
	s_objMapMgResponse.add(m_connection, this);
}

MgResponse::~MgResponse()
{
	s_objMapMgResponse.remove(m_connection);
}

void MgResponse::setHeader(int statusCode, const char *httpVersion, const char *content_type, unsigned long content_length, const char *transfer_encoding)
{
	m_statusCode	= statusCode;	
	m_httpVersion	= httpVersion;
	m_contentType	= content_type;
	m_contentLength	= content_length;
	m_transferEncoding = transfer_encoding;
}

void MgResponse::writeHeader(int statusCode, const char *httpVersion, const char *content_type, unsigned long content_length, const char *transfer_encoding)
{
	mg_printf(m_connection, 
		"%s %d %s\r\n"
		"Cache: %s\r\n"
		"Content-Type: %s\r\n"
		,		
		httpVersion, statusCode, 200 == statusCode ? "OK" : "Error",
		m_cache ? "" : "no-cache",
		content_type);
	if (content_length > 0) mg_printf(m_connection, "Content-Length: %d\r\n", content_length);
	if (transfer_encoding) mg_printf(m_connection, "Transfer-Encoding: %s\r\n", transfer_encoding);		
	mg_printf(m_connection, "\r\n");
}

void MgResponse::write(const std::string &content)
{
	mg_write(m_connection, content.c_str(), content.length());
}

void MgResponse::end()
{
	mg_printf(m_connection, "");
}


                                         
/**
 *  _____     _____                     _   
 * |     |___| __  |___ ___ _ _ ___ ___| |_ 
 * | | | | . |    -| -_| . | | | -_|_ -|  _|
 * |_|_|_|_  |__|__|___|_  |___|___|___|_|  
 *       |___|           |_|                
 */

MgRequest::MgRequest(mg_connection *con)
	: m_connection(con)
{
	s_objMapMgRequest.add(m_connection, this);
}

MgRequest::~MgRequest() 
{
	s_objMapMgRequest.remove(m_connection);
}

int MgRequest::read(char *buf, size_t len)
{
	return mg_read(m_connection, buf, len);
}

std::string MgRequest::requestMethod() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->request_method == NULL ? "" : info->request_method;
	return "";
}

std::string MgRequest::uri() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->uri == NULL ? "" : info->uri;
	return "";
}

std::string MgRequest::httpVersion() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->http_version == NULL ? "" : info->http_version;
	return "";
}

std::string MgRequest::queryString() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->query_string == NULL ? "" : info->query_string;
	return "";
}

std::string MgRequest::remoteUser() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->remote_user == NULL ? "" : info->remote_user;
	return "";
}

long MgRequest::remoteIP() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->remote_ip;
	return 0;
}

int MgRequest::remotePort() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->remote_port;
	return -1;
}

int MgRequest::is_ssl() const
{
	if (mg_request_info *info = mg_get_request_info(m_connection)) return info->is_ssl;
	return -1;
}
