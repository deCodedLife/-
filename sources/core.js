function request( url, callback ) { // Simple request function
    let xhr = new XMLHttpRequest()
    xhr.onreadystatechange = (function(myxhr) {
        return function() { if (myxhr.readyState == 4 && myxhr.status == 200) callback(myxhr) }
    })(xhr)
    xhr.open('GET', url, false)
    xhr.send('')
}

let coreUrl = "http://work-backend.000webhostapp.com/school/"   // host
var data = {}   // just working

function lib ( url ) { request( url, function (reqv) { data = reqv.responseText[0] == "{" ? JSON.parse(reqv.responseText) : reqv.responseText }); return data } // getting and tranforming data

function getToken( username, password ) { return lib( coreUrl + "access.php?name=" + username + "&pass=" + password ) }
function getMessages( token )           { return lib( coreUrl + "get-info.php?messages=1&token=" + token ) }
function getCurrentUser( user, token )  { return lib( coreUrl + "get-info.php?currentUser=" + user + "&token=" + token ) }
function getTags ( token )              { return lib( coreUrl + "get-info.php?tags=1&token=" + token ) }
function getTasks( token )              { return lib( coreUrl + "get-info.php?tasks=1&token=" + token ) }
function selectTask( id, token)         { return lib( coreUrl + "select-task.php?id=" + id + "&token=" + token ) }
function getImages ( tag, token )       { return lib( coreUrl + "get-images.php?tag=" + tag + "&token=" + token ) }
function getImagesByDate( date, token ) { return lib( coreUrl + "get-images.php?date=" + date + "&token=" + token ) }

function delMessage( pick, message, operator, date, token ) { return lib( coreUrl + "delete-message.php?message=" + message + "&pick=" + pick + "&date=" + date + "&token=" + token ) }
function delTag( group, tag, token ) {  return lib( coreUrl + "delete-tag.php?tag=" + tag + "&group=" + group + "&token=" + token ) }
function delImage( tag, date, path, token ) { return lib( coreUrl + "delete-image.php?tag=" + tag + "&date=" + date + "&path=" + path + "&token=" + token ) }

function addTag    ( tagname, token )               { return lib( coreUrl + "add-tag.php?tag=" + tagname + "&token=" + token ) }
function addMessage( pick, message, token )         { return lib( coreUrl + "add-message.php?pick=" + pick + "&message=" + message + "&token=" + token ) }
function addTask   ( tag, task, date_to, token )    { return lib( coreUrl + "add-task.php?tag=" + tag + "&task=" + task + "&date_to=" + date_to + "&token=" + token ) }

function dataConvert(data) { data = data.split("-"); data = data[2] + "-" + data[1] + "-" + data[0]; return data }  // replacing data numbers
function inArray ( value, array ) {for ( let i = 0; i < array.length; i++ ) if ( value == array[i] ) return true; return false }    // just search in array
function grabFrom( value, array ) {let new_ = []; for ( let i = 0; i < array.length; i++ ) if ( value != i ) new_.push( array[i] ); return new_  }
function delFrom ( value, array ) {let new_ = []; for ( let i = 0; i < array.length; i++ ) if ( array[i] != value ) new_.push( array[i] ); return new_ }  // delete value from array

function checkConnection(){ let xhr = new XMLHttpRequest(); xhr.open( "HEAD", "//" + window.location.hostname + "/?rand=" + Math.floor((1 + Math.random()) * 0x10000), false ); try { xhr.send(); return ( xhr.status >= 200 && (xhr.status < 300 || xhr.status === 304) ); } catch (error) { return false; } }
// You will die, if you will want to understanding this code. BECAUSE I USUALLY WRITING "CORE JS CODE FILES" AS IS WRITTEN ABOVE
// Code maximum compressed
