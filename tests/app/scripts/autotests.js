var expect = chai.expect;


var apiList = [
               {request: '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "menu://"}',
                answer:  '{"data":{"menus":[{"enabled":true,"icon":"http://ip/imgs/library.png","name":"Library","url_scheme":"library://"},{"enabled":false,"icon":"http://ip/imgs/cdda.png","name":"Audio CD","url_scheme":"cdda://"},{"enabled":true,"icon":"http://ip/imgs/playlists.png","name":"Playlists","url_scheme":"playlist://"},{"enabled":true,"icon":"http://ip/imgs/settings.png","name":"Settings","url_scheme":"settings://"}]},"msg":"EMS_BROWSE","msg_id":"42","uuid":"110e8400-e29b-11d4-a716-446655440000"}'},
               {request: '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/artists"}',
                answer:  '{"data":{"menus":[{"enabled":true,"icon":"http://ip/imgs/library.png","name":"Library","url_scheme":"library://"},{"enabled":false,"icon":"http://ip/imgs/cdda.png","name":"Audio CD","url_scheme":"cdda://"},{"enabled":true,"icon":"http://ip/imgs/playlists.png","name":"Playlists","url_scheme":"playlist://"},{"enabled":true,"icon":"http://ip/imgs/settings.png","name":"Settings","url_scheme":"settings://"}]},"msg":"EMS_BROWSE","msg_id":"42","uuid":"110e8400-e29b-11d4-a716-446655440000"}'}
]

//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "cdda://"}',
//     
//     '',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/artists/1"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/artists/1/1"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/albums"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/albums/1"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/tracks"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/tracks/1"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/genres"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/genres/1"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "playlist://current"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "playlist://"}',
//     '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "playlist://1"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "next"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "prev"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play", "filename": "library://music/tracks/1"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play", "filename": "library://music/albums/1"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play", "pos": "1"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "pause"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "toggle"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "stop"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "shuffle_on"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "shuffle_off"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "repeat_on"}',
//     '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "repeat_off"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "create", "subdir": "playlists_dirname", "name": "my_new_playlist"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "cdda://"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "file:///media/usb/music/beatles"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "library://music/tracks/1"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "library://music/albums/1"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://current", "filename": "file:///media/usb/music/beatles"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "clear", "url": "playlist://current"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://2", "filename": "library://music/tracks/1"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://5", "filename": "library://music/tracks/55"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://0", "filename": "library://music/albums/10"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://0"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "load", "url": "playlist://0"}',
//     '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "save", "subdir": "playlists_dirname", "name": "saved_current_playlist"}',
//     '{ "msg": "EMS_AUTH", "msg_id": "42", "status": "accepted", "uuid": "110e8400-e29b-11d4-a716-446655440000", "hostname": "192.168.0.1", "username": "remoteClientName"}',
//     '{ "msg": "EMS_CD_RIP", "msg_id": "42", "audio_format": "FLAC"}',
// ];


describe("Ems Websocket server", function(){
        var opened = false;
        beforeEach(function(done){
                ws = new WebSocket('ws://127.0.0.1:7337');
                ws.onopen = function(evt) {
                    opened = true;
                    done();
                    ws.close();
                }
            });

        it("is opened", function(){
                expect(opened).equals(true);
            });

    });


function exeTests(element, index, array) {
describe("Ems Browse " + array[index].request.msg + " " + "|url : " +  array[index].request.url , function() {
        var data = {};
        beforeEach(function(done){
                ws = new WebSocket('ws://127.0.0.1:7337');
        
                ws.onopen = function(evt) {
                    console.log("Websocket opened");
                    ws.send(array[index].request);
                }

                ws.onmessage = function(evt) {
                    console.log(evt.data);
                    data = evt.data;
                    done();
                }
            });

        it("message received", function(){
                expect(data).equals(array[index].answer);
            });

    });

}

apiList.forEach(exeTests);
