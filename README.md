mongooseCpp
===========

C++ Class Library of mongoose

###Example Code
    MongooseCpp::ServerConfig cfg;
    cfg.document_root	= "html";
    #ifdef NO_SSL
    cfg.listening_ports = "80"
    #else
    cfg.listening_ports = "443s"
    cfg.ssl_certificate = "cert.pem";
    #endif
    MgServer *server = MongooseCpp::createServer(cfg);	 
    server->listen([](MgRequest *req, MgResponse *res) -> int{
        cout << req->url() << endl;
        cout << req->queryString() << endl;
        res->writeHeader(200, "HTTP/1.1", "text/plain", 0, "");
        res->write("Hello World!");
        res->end();
        return 0;
    });
test
