'use strict';

var apiList = [
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "menu://"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "cdda://"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/artists"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/artists/1"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/artists/1/1"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/albums"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/albums/1"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/tracks"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/tracks/1"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/genres"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "library://music/genres/1"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "playlist://current"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "playlist://"}',
    '{ "msg": "EMS_BROWSE", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "url": "playlist://1"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "next"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "prev"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play", "filename": "library://music/tracks/1"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play", "filename": "library://music/albums/1"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "play", "pos": "1"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "pause"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "toggle"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "stop"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "shuffle_on"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "shuffle_off"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "repeat_on"}',
    '{ "msg": "EMS_PLAYER", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "repeat_off"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "create", "subdir": "playlists_dirname", "name": "my_new_playlist"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "cdda://"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "file:///media/usb/music/beatles"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "library://music/tracks/1"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://current", "filename": "library://music/albums/1"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://current", "filename": "file:///media/usb/music/beatles"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "clear", "url": "playlist://current"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "add", "url": "playlist://2", "filename": "library://music/tracks/1"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://5", "filename": "library://music/tracks/55"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://0", "filename": "library://music/albums/10"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "del", "url": "playlist://0"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "load", "url": "playlist://0"}',
    '{ "msg": "EMS_PLAYLIST", "msg_id": "42", "uuid": "110e8400-e29b-11d4-a716-446655440000", "action": "save", "subdir": "playlists_dirname", "name": "saved_current_playlist"}',
    '{ "msg": "EMS_AUTH", "msg_id": "42", "status": "accepted", "uuid": "110e8400-e29b-11d4-a716-446655440000", "hostname": "192.168.0.1", "username": "remoteClientName"}',
    '{ "msg": "EMS_CD_RIP", "msg_id": "42", "audio_format": "FLAC"}',
];

function popuplateApiList() {
    var i = 0;
    for (var c = 0;c < apiList.length;c++) {
        var s = JSON.parse(apiList[c]).msg
        var url = JSON.parse(apiList[c]).url
        var action = JSON.parse(apiList[c]).action
        if (url)
            s += ' | ' + url
        if (action)
            s += ' | ' + action
        $('#api_list').append($('<option />').val(i++).html(s));
    }

    $('#api_list').change(function() {
        var j = $('#api_list option:selected').val();
        var s = apiList[parseInt(j)];
        $('#message').val(JSON.stringify(JSON.parse(s), null, '    '));
    });

    $('#api_list').change();
}

var ws = new Object();
var loginfos = [];

function addLog(o, sending) {

    sending = typeof sending !== 'undefined' ? sending : false;

    var id = loginfos.length;
    var newItem = $('<p></p>');
    loginfos.push(o);

    $('.log').append(newItem);
    if (o.type == 'connected') {
        newItem.append($('<span class="glyphicon glyphicon-ok">'));
        newItem.append('Connected');
    }
    else if (o.type == 'disconnected') {
        newItem.append($('<span class="glyphicon glyphicon-remove">'));
        newItem.append('Disconnected');
    }
    else if (o.type == 'error') {
        newItem.append($('<span class="glyphicon glyphicon-remove-sign">'));
        newItem.append(o.type);
    }
    else if (sending) {
        newItem.append($('<span class="glyphicon glyphicon-export">'));
        if (o.type == '')
            newItem.append('Empty "msg"');
        else
            newItem.append(o.type);
    }
    else {
        newItem.append($('<span class="glyphicon glyphicon-import">'));
        if (o.type == '')
            newItem.append('Empty "msg"');
        else
            newItem.append(o.type);
    }

    $('.badge').html(loginfos.length);

    newItem.on('click', function () {
        var data = loginfos[id];
        var txt;
        try {
            txt = JSON.stringify(JSON.parse(data.json), null, '    ');
        }
        catch(ex) {
            txt = data.json;
        }

        $('#answer').html(txt);

        $('pre code').each(function(i, block) {
          hljs.highlightBlock(block);
        });
    });
}

function clearLog() {
    loginfos = [];
    $('.log').html('');
    $('.badge').html('0');
}

$(document).ready(function() {

    popuplateApiList();

    $('#urlinput').val('ws://127.0.0.1:7337');

    $('#btdisconnect').on('click', function () {
        ws.close();
    });

    $('#btsend').on('click', function () {

        var m = $('#message').val();

        if ('readyState' in ws &&
             ws.readyState == ws.OPEN) {
            ws.send(m);
        }
        else {
            console.log('sending error');
            addLog({
                type: 'error',
                msg_id: '',
                json: 'Websocket connection closed'
            });
            return;
        }

        var json = JSON.parse(m);

        var id = '';
        var msg = 'UNKNOWN!!';
        if (json.hasOwnProperty('msg_id'))
            id = json.msg_id;
        if (json.hasOwnProperty('msg'))
            msg = json.msg;

        addLog({
            type: msg,
            msg_id: id,
            json: m
        }, true);
    });

    $('#btclear').on('click', function () {
        clearLog();
    });

    $('#btconnect').on('click', function () {
        ws = new WebSocket($('#urlinput').val());
        ws.onopen = function(evt) {
            $('#status_con').removeClass('label-danger');
            $('#status_con').addClass('label-success');
            $('#status_con').html('Connected');

            addLog({
                type: 'connected',
                msg_id: '',
                json: 'OPEN'
            });
        };
        ws.onclose = function(evt) {
            $('#status_con').removeClass('label-success');
            $('#status_con').addClass('label-danger');
            $('#status_con').html('Disconnected');

            addLog({
                type: 'disconnected',
                msg_id: '',
                json: 'CLOSED'
            });
        };
        ws.onmessage = function(evt) {
            console.log(evt.data);

            var json = JSON.parse(evt.data);

            var id = '';
            var msg = 'UNKNOWN!!';
            if (json.hasOwnProperty('msg_id'))
                id = json.msg_id;
            if (json.hasOwnProperty('msg'))
                msg = json.msg;

            addLog({
                type: msg,
                msg_id: id,
                json: evt.data
            });
        };
        ws.onerror = function(evt) {
            $('#status_con').removeClass('label-success');
            $('#status_con').addClass('label-danger');
            $('#status_con').html('Disconnected');

            addLog({
                type: 'error',
                msg_id: '',
                json: evt.data
            });
        };
    });
});
