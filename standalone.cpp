#include "soapRManagerService.h"
#include "RManager.nsmap"
#include <iostream>
#include <pqxx/pqxx>
#include <signal.h>

void sigpipe_handle(int x) { }
int main(){
  signal(SIGPIPE,sigpipe_handle);
  RManagerService server;
  int m,s,t;
  soap_ssl_init();
  soap_init(&server);
  if(soap_ssl_server_context(&server,
        SOAP_SSL_REQUIRE_CLIENT_AUTHENTICATION | SOAP_TLSv1,
        "server.pem",
        NULL,
        "cacert.pem",
        NULL,
        NULL,
        NULL,
        NULL)){
    soap_print_fault(&server,stderr);
    exit(1);
  }
  m=server.bind(NULL,12345,100);
  if(m<0){
    soap_print_fault(&server,stderr);
  }
  else{
    std::cout<<"Socket connection successful: master socket = "<<m<<std::endl;
    for(int i=1;;i++){
      s=server.accept();
      if(s<0){
        soap_print_fault(&server,stderr);
        break;
      }
      std::cout<<i<<": accepted connection from IP="<<((int)(server.ip>>24)&0xFF)<<((int)(server.ip>>16)&0xFF)<<((int)(server.ip>>8)&0xFF)<<((int)(server.ip>>0)&0xFF)<<" socket="<<s<<std::endl;
      if(server.ssl_accept() != SOAP_OK){
        soap_print_fault(&server,stderr);
        break;
      }
      if(server.serve()!=SOAP_OK)
        soap_print_fault(&server,stderr);
      std::cout<<"request served\n";
    }
  }
  server.destroy();
}

int RManagerService::register_(ns1__transport *req, ns1__transport &res){
  std::vector<ns1__relation>::iterator itr;
  std::unique_ptr<pqxx::connection> conn(new pqxx::connection("dbname=test user=testuser password=testpass"));
  std::unique_ptr<pqxx::work> T(new pqxx::work(*conn));
  for(itr=req->body.relation.begin(); itr!=req->body.relation.end();itr++){
    T.get()->exec(
        "INSERT INTO server "
        "VALUES ("+std::to_string(itr->relation_id)
        +","+T.get()->quote(itr->service_id)
        +","+T.get()->quote(itr->node_id)
        +","+std::to_string(itr->privacy_lvl)
        +","+T.get()->quote(itr->anonymous_method)
        +","+std::to_string(itr->interval)+")");
  }
  T.get()->commit();
  res.header.massage="OK";
  return SOAP_OK;
}

int RManagerService::change(ns1__transport *req, ns1__transport &res){
  std::vector<ns1__relation>::iterator itr;
  std::unique_ptr<pqxx::connection> conn(new pqxx::connection("dbname=test user=testuser password=testpass"));
  std::unique_ptr<pqxx::work> T(new pqxx::work(*conn));
  for(itr=req->body.relation.begin(); itr!=req->body.relation.end();itr++){
    T.get()->exec(
        "UPDATE server SET privacy_lvl = " + std::to_string(itr->privacy_lvl)
        + " WHERE relationid = " + std::to_string(itr->relation_id));
  }
  T.get()->commit();
  res.header.massage="OK";
  return SOAP_OK;
}

int RManagerService::delete_(ns1__transport *req, ns1__transport &res){
  std::vector<ns1__relation>::iterator itr;
  std::unique_ptr<pqxx::connection> conn(new pqxx::connection("dbname=test user=testuser password=testpass"));
  std::unique_ptr<pqxx::work> T(new pqxx::work(*conn));
  for(itr=req->body.relation.begin(); itr!=req->body.relation.end();itr++){
    T.get()->exec(
        "DELETE FROM server"
        " WHERE relationid = " + std::to_string(itr->relation_id));
  }
  T.get()->commit();
  res.header.massage="OK";
  return SOAP_OK;
}

