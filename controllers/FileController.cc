#include "FileController.h"
#include "drogon/orm/Exception.h"
#include "drogon/orm/Result.h"

using namespace drogon;
void FileController::addFile(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> && callback)const{
    Json::Value message;
    MultiPartParser fileUpload;

    fileUpload.parse(req);
    auto para = fileUpload.getParameters();
    auto allFile = fileUpload.getFiles();    
    if(para.size()!= 2 || allFile.size() != 1){
        message["code"] = 1;
        message["error"] = "parameters error, add failed";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
        callback(resp);
        return;
    }

    auto &file = allFile[0];
    auto MD5 = file.getMd5();
    message["md5"] = MD5;

    std::string key = "path";
    std::string path = para[key];
    LOG_DEBUG<<"path:"<<path;

		std::string key2 = "userId";
		std::string temp = para[key2];
		int userId = 0;
		int lenUserId = temp.size();
		for(int i = 0; i < lenUserId; i++){
			userId = userId * 10 + temp[i] - '0';
		}
		LOG_ERROR << "userId is " << userId;

    std::string fileName = file.getFileName();
		std::string suffix = "";
		for(int i = fileName.size() - 1; i > 0; i--){
			suffix += fileName[i];
			if(fileName[i] == '.')
				break;
		}
		std::reverse(suffix.begin(), suffix.end());
		std::string rename = MD5 + suffix;
    auto dbclient = drogon::app().getDbClient();
    try{
        auto result = dbclient->execSqlSync("select * from file where MD5 = ?", MD5);
        int sum = result.size();
        if(sum == 0){
            file.saveAs(rename);
        //  dbclient->execSqlSync("insert into file(fileType, MD5) values(?, ?)", fileType, MD5);
            dbclient->execSqlSync("insert into file(MD5, fileExtension) values(?, ?)", MD5, suffix);
            result = dbclient->execSqlSync("select * from file where MD5 = ?", MD5);
        }
        
        int fileId;
        for(const auto &row : result){
            fileId = row["id"].as<int>();
        }
       
        auto fileEnum = file.getFileType();
        std::string fileType;
        switch (fileEnum){
            case 2 : fileType = "document"; break;
            case 3 : fileType = "archieve"; break;
            case 4 : fileType = "audio"; break;
            case 5 : fileType = "media"; break;
            case 6 : fileType = "image"; break;
            default : fileType = "unknown";
        } 
				auto query = dbclient->execSqlSync("select * from fileOfUser where user_id = ? and file_id = ?;", userId, fileId);
				if(query.size() == 0){
        	dbclient->execSqlSync("insert into fileOfUser values(?, ?, ?, ?, now() + interval 8 hour);", 
          userId, fileId, path, fileName);
				}else{
					message["warnning"] = "file has been uploaded";
				}
        message["code"] = "0";
    }catch(drogon::orm::DrogonDbException &e){
        message["error"] = "Add failed";
    }
    auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
    callback(resp);
}



void FileController::deleteFile(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> && callback, Json::Value json)const{
    auto dbclient = drogon::app().getDbClient();
    Json::Value message;
    int id = json["id"].as<int>();
    try{
        dbclient->execSqlSync("delete from file where id = ?;", id);
        message["code"] = 0;
    }catch (drogon::orm::DrogonDbException &e){
        message["error"] = "Delete failed";
    }
    auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
    callback(resp);
}
void FileController::findFileName(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> && callback, Json::Value json)const{
    auto dbclient = drogon::app().getDbClient();
    Json::Value message;
    try{
        std::string fileName = json["fileName"].as<std::string>();
        std::string sql = "select * from file where fileName = ?;";
        std::cout<<sql;
        auto future = dbclient->execSqlAsyncFuture(sql, fileName);
        auto result = future.get();
        for (const auto &row : result){
            Json::Value item;
            item["id"] = row["id"].as<int>();
            item["fileName"] = row["fileName"].as<std::string>();
            item["fileType"] = row["fileType"].as<std::string>();
            item["MD5"] = row["MD5"].as<std::string>();
            item["path"] = row["path"].as<std::string>();
            message.append(item);
        }
    }catch (drogon::orm::DrogonDbException &e){
        message["error"] = "find file by Name failed";
    }
    auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
    callback(resp);
}
void FileController::findFileMD5(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> && callback, Json::Value json)const{
    auto dbclient = drogon::app().getDbClient();
    Json::Value message;
    try{
        std::string MD5 = json["MD5"].as<std::string>();
        std::string sql = "select * from file where MD5 = ?;";
        auto future = dbclient->execSqlAsyncFuture(sql, MD5);
        auto result = future.get();
        for (const auto &row : result){
            Json::Value item;
            item["id"] = row["id"].as<int>();
            item["fileName"] = row["fileName"].as<std::string>();
            item["fileType"] = row["fileType"].as<std::string>();
            item["MD5"] = row["MD5"].as<std::string>();
            item["path"] = row["path"].as<std::string>();
            message.append(item);
        }
    }catch (drogon::orm::DrogonDbException &e){
        message["error"] = "find file by MD5 failed";
    }
    auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
    callback(resp);
}
void FileController::listFile(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> && callback, Json::Value json)const{
    auto dbclient = drogon::app().getDbClient();
    Json::Value message;
    try{
        std::string path = json["path"].as<std::string>();
        std::string userId = json["userId"].as<std::string>();
        std::string sql = "select * from fileOfUser where userId=? AND path = ?;";
        auto future = dbclient->execSqlAsyncFuture(sql, userId, path);
        auto result = future.get();
        for (const auto &row : result){
            Json::Value item;
            item["fileId"] = row["fileId"].as<std::string>();
            item["fileName"] = row["fileName"].as<std::string>();
            item["time"] = row["time"].as<std::string>();
            item["path"] = row["path"].as<std::string>();
            message.append(item);
        }
    }catch (drogon::orm::DrogonDbException &e){
        LOG_DEBUG<<e.base().what();
        message["error"] = "list failed";
    }
    auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
    callback(resp);
}

void FileController::downLoadFile(const HttpRequestPtr& req, 
    std::function<void (const HttpResponsePtr &)> &&callback, 
    Json::Value json
) const {
    auto dbclient = drogon::app().getDbClient();
    Json::Value message;
    try{
        std::string MD5 = json["MD5"].as<std::string>();
        std::string sql = "select * from file where MD5 = ?";
        auto future = dbclient->execSqlAsyncFuture(sql, MD5);
        auto result = future.get();
        if(result.empty()) {
            message["error"] = "download failed";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
            callback(resp);
            return ;
        }
    }catch (drogon::orm::DrogonDbException &e){
        message["error"] = "download failed";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
        callback(resp);
        return ;
    }

    // auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    // callback(resp);
    auto resp = drogon::HttpResponse::newFileResponse("./uploads/Screenshot_2023-11-10_10-20-01.png");
    callback(resp);
}

void FileController::downLoadFileGet(const HttpRequestPtr& req, 
    std::function<void (const HttpResponsePtr &)> &&callback,
    std::string userId,
    std::string fileId
) const{
    auto dbclient = drogon::app().getDbClient();
    Json::Value message;
    std::string MD5, suffix;
    try{
        std::string sql = 
        " \
            SELECT f.MD5 \
            FROM fileOfUser fu \
            JOIN file f ON fu.fileId = f.fileId \
            WHERE fu.userId = ? AND fu.fileId = ?; \
        ";
        auto future = dbclient->execSqlAsyncFuture(sql, userId, fileId);
        auto result = future.get();
        if(result.empty()) {
            message["error"] = "download failed";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
            callback(resp);
            return ;
        }
        MD5 = result.at(0)["MD5"].as<std::string>();
        suffix = result.at(0)["suffix"].as<std::string>();
    }catch (drogon::orm::DrogonDbException &e){
        message["error"] = "download failed";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(message);
        callback(resp);
        return ;
    }

    // auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    // callback(resp);
    auto resp = drogon::HttpResponse::newFileResponse("./uploads/"+MD5+"."+suffix);
    callback(resp);

}
