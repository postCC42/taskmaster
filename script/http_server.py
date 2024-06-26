import os
from http.server import SimpleHTTPRequestHandler, HTTPServer

class CustomHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        greeting = os.getenv('GREETING', 'Hello, World!')
        authors = os.getenv('AUTHORS', 'unknown authors')
        message = f"{greeting}<br>This Taskmaster has been created by {authors}"
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write(message.encode("utf-8"))

if __name__ == "__main__":
    port = 8000
    server_address = ('', port)
    httpd = HTTPServer(server_address, CustomHandler)
    print(f"Starting server on port {port}...")
    httpd.serve_forever()