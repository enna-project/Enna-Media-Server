# Test page for Websocket server

  1. Launch enna-media-server : it creates a websocket on 127.0.0.1:7337
  2. Launch a webserver on this directory : python -m SimpleHTTPServer 8000
  3. Open Firefox or Chromium on this address : http://127.0.0.1:8000
  4. Send message and verify websocket server answer
