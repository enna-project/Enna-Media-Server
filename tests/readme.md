# Test page for Websocket server

  1. Launch enna-media-server : it creates a websocket on 127.0.0.1:7337
  2. Launch a webserver on  directory dist : cd dist && python -m SimpleHTTPServer 8000
  3. Open Firefox or Chromium at this address : http://127.0.0.1:8000
  4. Send message and verify websocket server answer

# For developping Debug tool : 

Install npm and grunt.

  1. npm install
  2. bower install
  3. grunt serve

Before committing: grunt clean && grunt build