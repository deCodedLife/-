#include "appcore.h"

#include <iostream>
using namespace std;

AppCore::AppCore( QObject *parent ) : QObject (parent) {    // loading
    int connection = checkConnection();
    if ( !QDir("data").exists() ) QDir().mkdir("data");
    database.setDatabaseName("data/database.bin");
    if ( !database.open() ) { qDebug() << database.lastError().text();  return; }
    else {
        if ( !database.tables().contains( QLatin1String("user_data") ) ) {    // if data base not correct
            QSqlQuery query;
            query.exec("CREATE TABLE user_data ( username  TEXT NOT NULL, password TEXT NOT NULL, status TEXT NOT NULL, fgroup TEXT NOT NULL, is_curator INTEGER NOT NULL, profile TEXT NOT NULL);"); // creating user_data table
            query.exec("CREATE TABLE images ( id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, groups TEXT NOT NULL, tag TEXT NOT NULL, date TEXT NOT NULL, path TEXT NOT NULL);");  // creating images table
            query.exec("CREATE TABLE messages ( id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, groups TEXT NOT NULL, pick INTEGER NOT NULL, message TEXT NOT NULL, operator TEXT NOT NULL, date TEXT NOT NULL);");  // creating messages table
            query.exec("CREATE TABLE tags ( id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, groups TEXT NOT NULL, tag TEXT NOT NULL, static INTEGER NOT NULL);");   // cteating tags table
            query.exec("CREATE TABLE config ( first INTEGER NOT NULL, theme INTEGER NOT NULL, trafic INTEGET NOT NULL);");   // creating config table
            query.exec("CREATE TABLE Tasks ( id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, groups TEXT NOT NULL, tag TEXT NOT NULL, task TEXT NOT NULL, attached TEXT NOT NULL, date_to TEXT NOT NULL, operator TEXT NOT NULL, finished INTEGER NOT NULL);");
            query.exec("INSERT INTO user_data (`username`,`password`,`status`,`fgroup`,`is_curator`,`profile`) VALUES (\"\",\"\",\"\",\"\",1,\"\")");  // creating start values
            query.exec("INSERT INTO config (`first`, `theme`, `trafic`) VALUES (1,0,0)");   // set default values
            query.clear();
        }
    }
    QJsonObject userdata = loadUserData();
    if ( connection == 0 && userdata["username"].toString() != "" ) token = get( "access.php?name=" + userdata["username"].toString() + "&pass=" + userdata["password"].toString() );
}

QString AppCore::get (QString url) {    // GET func
    //qDebug() << api + url;
    if ( checkable == 1 )  netRequest.setUrl( QUrl( url ) );// set url
    else netRequest.setUrl( QUrl( api + url ) );// set url
    netReply = netManager->get( netRequest );    // download page with GET method
    netManager->connect( netManager, SIGNAL( finished( QNetworkReply* ) ), &loop, SLOT(quit())); // When page downloaded, programm will exit from loop
    loop.exec();    // starting download page
    if ( checkable == 1 ) userIco = netReply->readAll();
    QString result = netReply->readAll(); // getting text from page
    return result;  // returning text from page
}
void AppCore::update ( QString username, QString password ) {
    QString token = getToken( username, password );
    if ( token.length() > 50 ) {
        QString url;
        if ( !database.open() ) qDebug() << "upload func: [ERROR] > " + database.lastError().text();
        else {
            QSqlQuery query;
            url = "get-info.php?profile=1&token=" + token;  // set url for get user info from network
            QString info = get( url );  // get profile info
            QJsonObject user_data = QJsonDocument::fromJson( info.toUtf8() ).object();  // convert to json object
            query.exec("UPDATE user_data SET `username` = \""+username+"\", `password` = \""+password+"\", `status` = \""+user_data["status"].toString()+"\", `fgroup` = \""+user_data["group"].toString()+"\",`is_curator` = \""+QString::number(user_data["is_curator"].toInt())+"\" , `profile` = \""+user_data["profile"].toString()+"\" "); // updating user data table

            url = "get-info.php?group=1";
            info = get( url ); // getting groups
            QJsonObject groups = QJsonDocument::fromJson( info.toUtf8() ).object();
            query.exec("DELETE FROM tags");     //clear tags
            query.exec("DELETE FROM images");   //clear images
            query.exec("DELETE FROM messages"); //clear messages
            query.exec("DELETE FROM Tasks");    //clear tasks


            url = "get-info.php?tags=1&token=" + token;
            info = get( url );   // get tags from USER group
            QJsonObject tags = QJsonDocument::fromJson( info.toUtf8() ).object();

            for ( int i = 0; i < tags["tags"].toArray().size(); i++ ) {
                QJsonObject item = tags["tags"].toArray().at(i).toObject();

                url = "get-images.php?tag=" + item["tag"].toString() + "&token=" + token;
                info = get( url );   // get images
                QJsonObject images = QJsonDocument::fromJson( info.toUtf8() ).object();

                for ( int i = 0; i < images["images"].toArray().count(); i++ ) {
                    QJsonObject image = images["images"].toArray().at(i).toObject();
                    query.exec("INSERT INTO images (`groups`,`tag`,`date`,`path`) VALUES (\""+user_data["group"].toString()+"\",\""+item["tag"].toString()+"\",\""+image["date"].toString()+"\",\""+image["image"].toString()+"\")"); // update images
                }

                query.exec("INSERT INTO tags (`groups`, `tag`, `static`) VALUES (\""+user_data["group"].toString()+"\",\""+item["tag"].toString()+"\","+QString::number(item["static"].toInt())+")");  // update tags
            }

            url = "get-info.php?messages=1&token=" + token;
            info = get( url );   // get messages
            QJsonObject messages = QJsonDocument::fromJson( info.toUtf8() ).object();

            for ( int i = 0; i < messages["messages"].toArray().count(); i++ ) {
                QJsonObject message = messages["messages"].toArray().at(i).toObject();
                query.exec("INSERT INTO messages (`groups`,`pick`,`message`,`operator`,`date`) VALUES (\""+user_data["group"].toString()+"\","+QString::number(message["pick"].toInt())+",\""+message["message"].toString()+"\",\""+message["operator"].toString()+"\",\""+message["date"].toString()+"\")"); // update messages
            }

            url = "get-info.php?tasks=1&token=" + token;
            info = get(url);
            QJsonObject tasks = QJsonDocument::fromJson( info.toUtf8() ).object();

            for ( int i = 0; i < tasks["tasks"].toArray().count(); i++ ) {
                QJsonObject task = tasks["tasks"].toArray().at(i).toObject();
                query.exec("INSERT INTO Tasks (`id`,`groups`,`tag`,`task`,`attached`,`date_to`,`operator`,`finished`) VALUES ("+QString::number(task["id"].toInt())+",\""+task["group"].toString()+"\", \""+task["tag"].toString()+"\",\""+task["task"].toString()+"\",\""+task["attached"].toString()+"\", \""+task["date_to"].toString()+"\", \""+task["operator"].toString()+"\", "+QString::number(task["finished"].toInt())+")");
            }

            query.clear();
        }
    }

}
QString AppCore::getToken(QString username, QString password)
{
    return get ( "access.php?name=" + username + "&pass=" + password );
}
QJsonObject AppCore::loadUserData () {  // get user data from db
    QJsonObject data;
    data["debug"] = 1;
    if ( !database.open() ) { qDebug() << database.lastError().text(); return data; }
    else {
        QSqlQuery query;
        query.exec("SELECT * FROM `user_data`");  // get user data from database
        if ( query.next() ) {
            data["username"] = query.value(0).toString();    // get username
            data["password"] = query.value(1).toString();    // get password
            data["status"] = query.value(2).toString();  // get status
            data["group"]  = query.value(3).toString();  // get group
            data["is_curator"] = query.value(4).toString();
            data["profile"]= query.value(5).toString();  // get ico
            data["debug"]  = "0";    // if all is ok, set zero value
        }
    }
    return  data;
}
QJsonObject AppCore::getConfig () { // get config values from db
    QJsonObject data;
    data["debug"] = "1";
    if ( !database.open() ) { qDebug() << database.lastError().text(); return data; }  // if error send 1 in debug
    else {
        QSqlQuery query;
        query.exec("SELECT * FROM `config`");   // reading config table
        if ( query.next() ) {
            data["first"] = query.value(0).toInt();
            data["theme"] = query.value(1).toInt();
            data["trafic"]= query.value(2).toInt();
            data["debug"] = "0";
        }
        query.clear();
    }
    return data;
}
QJsonArray AppCore::getMessages () {    // get messages from db
    QJsonArray data;
    QJsonObject sub;
    if ( !database.open() ) { qDebug() << database.lastError().text(); sub["debug"] = "1"; data.append(sub); }
    else {
        QSqlQuery query;
        query.exec("SELECT * FROM MESSAGES");
        while ( query.next() ) {
            sub["pick"] = query.value(2).toInt();
            sub["message"] = query.value(3).toString();
            sub["operator"] = query.value(4).toString();
            sub["date"] = query.value(5).toString();
            sub["debug"] = "0";
            data.append( sub );
        }
        query.clear();
    }
    return data;
}
QJsonArray AppCore::getTags () {    // get tags from db
    QJsonArray data;
    QJsonObject sub;
    if ( !database.open() ) { qDebug() << database.lastError().text(); sub["debug"] = "1"; data.append(sub); }
    else {
        QSqlQuery query;
        query.exec("SELECT * FROM tags");
        while ( query.next() ) {
            sub["tag"] = query.value(2).toString();
            sub["static"] = query.value(3).toString();
            sub["debug"] = "0";
            data.append( sub );
        }
        query.clear();
    }
    return data;
}
QJsonArray AppCore::getImages ( QString tag ) { // get images from db
    QJsonArray data;
    QJsonObject sub;
    if ( !database.open() ) { qDebug() << database.lastError().text(); sub["debug"] = "1"; data.append(sub); }
    else {
        QSqlQuery query;
        query.exec("SELECT * FROM images WHERE `tag` = \""+tag+"\"");
        while ( query.next() ) {
            sub["tag"]  = query.value(2).toString();
            sub["date"] = query.value(3).toString();
            sub["image"]= query.value(4).toString();
            sub["debug"] = "0";
            data.append( sub );
        }
        query.clear();
    }
    return data;
}
QJsonArray AppCore::getTasksDb() {
    QJsonArray data;
    QJsonObject sub;
    if ( !database.open() ) { qDebug() << database.lastError().text(); sub["debug"] = "1"; data.append(sub); }
    else {
        QSqlQuery query;
        query.exec("SELECT * FROM Tasks");
        while ( query.next() ) {
            sub["id"]   = query.value(0).toString();
            sub["group"]= query.value(1).toString();
            sub["tag"]  = query.value(2).toString();
            sub["task"] = query.value(3).toString();
            sub["attached"] = query.value(4).toString();
            sub["date_to"]  = query.value(5).toString();
            sub["operator"] = query.value(6).toString();
            sub["finished"] = query.value(7).toString();
            sub["debug"] = "0";
            data.append( sub );
        }
        query.clear();
    }
    return data;
}
void AppCore::uMessages () {  // update just messages
    if ( !database.open() ) { qDebug() << database.lastError().text(); }
    else {
        QSqlQuery query;
        QJsonObject uData = loadUserData();
        QString url = "get-info.php?messages=1&token=" + token;
        QJsonObject data = QJsonDocument::fromJson( get(url).toUtf8() ).object();
        query.exec("DELETE FROM messages");
        for ( int i = 0; i < data["messages"].toArray().size(); i++ ) {
            QJsonObject temp = data["messages"].toArray().at(i).toObject();
            query.exec("INSERT INTO messages (`groups`, `pick`, `message`, `operator`, `date`) VALUES (\""+uData["group"].toString()+"\", "+QString::number(temp["pick"].toInt())+", \""+temp["message"].toString()+"\", \""+temp["operator"].toString()+"\", \""+temp["date"].toString()+"\")");
        }
        query.clear();
    }
}
void AppCore::uTags () {  // update just tags
    if ( !database.open() ) { qDebug() << database.lastError().text(); }
    else {
        QSqlQuery query;
        QJsonObject uData = loadUserData();
        QString url = "get-info.php?tags=1&token=" + token;
        QJsonObject data = QJsonDocument::fromJson( get(url).toUtf8() ).object();
        query.exec("DELETE FROM tags");
        for ( int i = 0; i < data["tags"].toArray().size(); i++ ){
            QJsonObject temp = data["tags"].toArray().at(i).toObject();
            query.exec("INSERT INTO tags (`groups`, `tag`, `static`) VALUES (\""+uData["group"].toString()+"\", \""+temp["tag"].toString()+"\", "+QString::number(temp["static"].toInt())+")");
        }
        query.clear();
    }
}
void AppCore::uImages () {  // update just images
    if ( !database.open() ) { qDebug() << database.lastError().text(); }
    else {
        QSqlQuery query;
        QJsonObject user_data = loadUserData();

        QString url = "get-info.php?tags=1&token=" + token;
        QString info = get( url );   // get tags from USER group
        QJsonObject tags = QJsonDocument::fromJson( info.toUtf8() ).object();


        query.exec("DELETE FROM images");
        query.exec("DELETE FROM tags");
        for ( int i = 0; i < tags["tags"].toArray().size(); i++ ) {
            QJsonObject item = tags["tags"].toArray().at(i).toObject();

            url = "get-images.php?tag=" + item["tag"].toString() + "&token=" + token;
            info = get( url );   // get images
            QJsonObject images = QJsonDocument::fromJson( info.toUtf8() ).object();

            for ( int i = 0; i < images["images"].toArray().count(); i++ ) {
                QJsonObject image = images["images"].toArray().at(i).toObject();
                query.exec("INSERT INTO images (`groups`,`tag`,`date`,`path`) VALUES (\""+user_data["group"].toString()+"\",\""+item["tag"].toString()+"\",\""+image["date"].toString()+"\",\""+image["image"].toString()+"\")"); // update images
            }

            query.exec("INSERT INTO tags (`groups`, `tag`, `static`) VALUES (\""+user_data["group"].toString()+"\",\""+item["tag"].toString()+"\","+QString::number(item["static"].toInt())+")");  // update tags
        }
        query.clear();
    }
}
void AppCore::uTasks() {
    if ( !database.open() ) { qDebug() << database.lastError().text(); }
    else {
        QSqlQuery query;
        QJsonObject uData = loadUserData();
        query.exec("DELETE FROM Tasks");
        QString url = "get-info.php?tasks=1&token=" + token;
        QString info = get(url);
        QJsonObject tasks = QJsonDocument::fromJson( info.toUtf8() ).object();
        for ( int i = 0; i < tasks["tasks"].toArray().count(); i++ ) {
            QJsonObject task = tasks["tasks"].toArray().at(i).toObject();
            //qDebug() << "INSERT INTO Tasks (`id`,`groups`,`tag`,`task`,`attached`,`date_to`,`operator`,`finished`) VALUES ("+QString::number(task["id"].toInt())+",\""+task["group"].toString()+"\", \""+task["tag"].toString()+"\",\""+task["task"].toString()+"\",\""+task["attached"].toString()+"\", \""+task["date_to"].toString()+"\", \""+task["operator"].toString()+"\", "+QString::number(task["finished"].toInt())+")";
            query.exec("INSERT INTO Tasks (`id`,`groups`,`tag`,`task`,`attached`,`date_to`,`operator`,`finished`) VALUES ("+QString::number(task["id"].toInt())+",\""+task["group"].toString()+"\", \""+task["tag"].toString()+"\",\""+task["task"].toString()+"\",\""+task["attached"].toString()+"\", \""+task["date_to"].toString()+"\", \""+task["operator"].toString()+"\", "+QString::number(task["finished"].toInt())+")");
        }
        query.clear();
    }
}
void AppCore::logout() {
    if ( !database.open() ) { qDebug() << database.lastError().text(); }
    else {
        QSqlQuery query;
        query.exec("DELETE FROM tags");     //clear tags
        query.exec("DELETE FROM images");   //clear images
        query.exec("DELETE FROM messages"); //clear messages
        query.exec("DELETE FROM Tasks");    //clear tasks
        query.exec("DELETE FROM user_data");//clear userdata
        query.exec("INSERT INTO user_data (`username`,`password`,`status`,`fgroup`,`is_curator`,`profile`) VALUES (\"\",\"\",\"\",\"\",1,\"\")");  // creating start values
    }
}
void AppCore::updateUser() {
    if ( !database.open() ) { qDebug() << database.lastError().text(); }
    else {
        QSqlQuery query;
        QJsonObject user_data = getMyProfile();
        QJsonObject user_base = loadUserData();
        query.exec("DELETE FROM user_data");//clear userdata
        query.exec("INSERT INTO user_data (`username`,`password`,`status`,`fgroup`,`is_curator`,`profile`) VALUES (\"\",\"\",\"\",\"\",1,\"\")");  // creating start values
        query.exec("UPDATE user_data SET `username` = \""+user_base["username"].toString()+"\", `password` = \""+user_base["password"].toString()+"\", `status` = \""+user_data["status"].toString()+"\", `fgroup` = \""+user_data["group"].toString()+"\",`is_curator` = \""+QString::number(user_data["is_curator"].toInt())+"\" , `profile` = \""+user_data["profile"].toString()+"\" "); // updating user data table
    }
}
QString AppCore::sendImage ( QString files, QString data ) {    // send image to server
    QString filepath = files;
    QString filename = QFileInfo(filepath).fileName();

    QHttpMultiPart * multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name= \"image\"; filename=\""+filepath+"\""));

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"name\""));
    textPart.setBody(filename.toUtf8());

    QFile *file = new QFile(filepath);
    file->open(QIODevice::ReadOnly);
    imagePart.setBodyDevice(file);
    file->setParent(multipart);

    multipart->append(textPart);
    multipart->append(imagePart);

    QNetworkRequest newNetRequst(QUrl(api + data + "&path=" + filename + "&token=" + token ));
    QNetworkAccessManager * newNetManager = new QNetworkAccessManager;
    QNetworkReply *newNetReply = newNetManager->post(newNetRequst, multipart);
    multipart->setParent(newNetReply);

    newNetManager->connect(newNetManager, SIGNAL(finished(QNetworkReply*)), &loop, SLOT(quit()));
    loop.exec();

    return newNetReply->readAll();
}
QString AppCore::getCuratorTag(QString tag) {
    return get( "get-curatorTag.php?tag=" + tag + "&token=" + token );
}
int AppCore::checkConnection () {    // checking for internet connection
    netRequest.setUrl( QUrl("http://vk.com") );
    netReply = netManager->get( netRequest );
    netManager->connect( netManager, SIGNAL( finished( QNetworkReply* ) ), &loop, SLOT( quit() ) );
    loop.exec();
    if ( netReply->bytesAvailable() || netReply->readAll() != "" ) return 0;  // if get no data then return error
    else return 1;
}
QJsonObject AppCore::userImage ( QString url ) {    // get somebody profile ico
    QJsonObject obj;
    QString patientDbPath = "";
    QFile file(":/profile.jpg");
    if ( file.exists() ) {
        patientDbPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if ( patientDbPath.isEmpty() ) {
            qDebug() << "Could not obtain writable location.";
            obj["debug"] = 1;
            return obj;
        }
        patientDbPath.append("/profile.jpg");
        QJsonObject user_data = loadUserData();
        if ( !QFile( patientDbPath ).exists() || user_data["profile"] != url ) {
            file.copy( patientDbPath );
            QFile::setPermissions(patientDbPath ,QFile::WriteOwner | QFile::ReadOwner) ;
            QFile current( patientDbPath );
            current.open(QIODevice::WriteOnly);
            checkable = 1;
            get( url );
            current.write( userIco );
            current.close();
        }
        obj["path"] = "file:///" + patientDbPath;
        obj["debug"]= 0;

    }
    checkable = 0;
    return obj;
}
QString AppCore::sendTag ( QString tagName, QString curator, QString group ) { // add new tag
    if ( curator != "" ) return get( "add-tag.php?tag=" + tagName + "&curator=" + curator + "&group=" + group + "&token=" + token );
    return get("add-tag.php?tag=" + tagName + "&token=" + token );
}
QString AppCore::sendMessage (QString pick, QString message) {   // add new message
    return get("add-message.php?pick=" + pick + "&message=" + message + "&token=" + token);
}
QString AppCore::sendTask(QString group, QString tagName, QString task, QString date_to) {
    return get("add-task.php?group=" + group + "&tag=" + tagName + "&task=" + task + "&date_to=" + date_to + "&token=" + token);
}
QString AppCore::deleteTag (QString group, QString tag ) {   // delete tag
    return get( "delete-tag.php?tag=" + tag + "&group=" + group + "&token=" + token );
}
QString AppCore::deleteImage ( QString path) { // delete Image
    return get( "delete-image.php?path=" + path + "&token=" + token );
}
QString AppCore::deleteMessage ( QString pick, QString message, QString date) {    // delete Message
    return get( "delete-message.php?message=" + message + "&pick=" + pick + "&date=" + date + "&token=" + token );
}
QString AppCore::deleteTask(QString id) {
    return get( "delete-task.php?id=" + id + "&token=" + token );
}
QString AppCore::deleteUser(QString username) {
    return get( "delete-user.php?username=" + username + "&token=" + token );
}
QString AppCore::selectTask( QString id ) {
    return get( "select-task.php?id=" + id + "&token=" + token );
}
QString AppCore::applyTask(QString id){
    return get( "apply-task.php?id=" + id + "&token=" + token );
}
QString AppCore::changeGroup(QString username, QString group) {
    return get( "changeGroup.php?username=" + username + "&group=" + group + "&token=" + token );
}
QString AppCore::newUser(QString username, QString group, QString status) {
    return get( "signup.php?name=" + username + "&status=" + status + "&group=" + group + "&token=" + token );
}
QString AppCore::selectStudent(QString id, QString user) {
    return get( "select-student.php?id=" + id + "&student=" + user + "&token=" + token );
}
QJsonObject AppCore::getTasks( QString args ) {
    return QJsonDocument::fromJson( get( "get-info.php?tasks=1&token=" + token + args ).toUtf8() ).object();
}
QJsonObject AppCore::getMyProfile() {
    return QJsonDocument::fromJson( get( "get-info.php?profile=1&token=" + token ).toUtf8() ).object();
}
QJsonObject AppCore::getCurator() {
    return QJsonDocument::fromJson( get( "getCurator.php?token=" + token ).toUtf8() ).object();
}
QJsonObject AppCore::igetGroups() {
    return QJsonDocument::fromJson( get( "get-info.php?groups=1" ).toUtf8() ).object();
}
QJsonObject AppCore::igetTags( int all, QString group ) {
    if ( group != "" )   return QJsonDocument::fromJson( get( "get-info.php?tags=1&group=" + group + "&token=" + token ).toUtf8() ).object();
    else if ( all != 0 ) return QJsonDocument::fromJson( get( "get-info.php?tags=1&token=" + token + "&all=1" ).toUtf8() ).object();
    else return QJsonDocument::fromJson( get( "get-info.php?tags=1&token=" + token ).toUtf8() ).object();
}
QJsonObject AppCore::igetImages(QString tag, QString date) {
    if ( date != "" ) return QJsonDocument::fromJson( get( "get-images.php?date=" + date + "&token=" + token ).toUtf8() ).object();
    else return QJsonDocument::fromJson( get( "get-images.php?tag=" + tag + "&token=" + token ).toUtf8() ).object();
}
QJsonObject AppCore::getProfile( QString username ) {
    if ( username != "" )
        return QJsonDocument::fromJson( get( "get-info.php?profile=" + username + "&token=" + token ).toUtf8() ).object();
    else
        return QJsonDocument::fromJson( get( "get-info.php?profile=1&token=" + token ).toUtf8() ).object();
}
QJsonObject AppCore::getUsers( int s, QString group ) {
    QJsonObject object;
    if ( group != "" ) group = "&group=" + group;
    object["debug"] = 1;
    if ( checkConnection() == 0 ) {
        if ( s == 1 )      object = QJsonDocument::fromJson( get ( "get-info.php?users=1&s=1" + group ).toUtf8() ).object();
        else if ( s == 2 ) object = QJsonDocument::fromJson( get ( "get-info.php?users=1&c=1" ).toUtf8() ).object();
        else if ( s == 3 ) object = QJsonDocument::fromJson( get ( "get-info.php?users=1&a=1" ).toUtf8() ).object();
        object["debug"] = 0;
    }
    return object;
}
QJsonObject AppCore::getUser(QString username) {
    return QJsonDocument::fromJson( get( "get-info.php?currentUser=" + username + "&token=" + token ).toUtf8() ).object();
}
QJsonObject AppCore::profileImage(QString url) {
    QJsonObject obj;
    QString patientDbPath = "";
    QFile file(":images/profile.jpg");
    if ( file.exists() ) {
        patientDbPath = QDir().absolutePath();
        if ( patientDbPath.isEmpty() ) {
            qDebug() << "Could not obtain writable location.";
            obj["debug"] = 1;
            return obj;
        }
        patientDbPath.append("/data/profile.jpg");
        QJsonObject user_data = loadUserData();
        if ( !QFile( patientDbPath ).exists() || user_data["profile"] != url ) {
            file.copy( patientDbPath );
            QFile::setPermissions(patientDbPath ,QFile::WriteOwner | QFile::ReadOwner) ;
            QFile current( patientDbPath );
            current.open(QIODevice::WriteOnly);
            checkable = 1;
            get( url );
            current.write( userIco );
            current.close();
        }
        obj["path"] = patientDbPath;
        obj["debug"]= 0;

    }
    checkable = 0;
    return obj;
}
void AppCore::close() {
    token = "";
    database.close();
}
