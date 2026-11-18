"""Локальные проверки командной строки на реальных TCP-соединениях."""

import os
from contextlib import contextmanager
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import socketserver
import subprocess
import sys
import threading
import unittest


MONITOR = Path(__file__).with_name("monitor.py")


class HealthHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/health":
            status = self.server.health_status
        else:
            status = 404
        self.send_response(status)
        self.end_headers()

    def log_message(self, format, *args):
        pass


class SilentHandler(socketserver.BaseRequestHandler):
    def handle(self):
        self.request.recv(4096)
        self.server.finish.wait()


def http_service(status=200):
    server = ThreadingHTTPServer(("127.0.0.1", 0), HealthHandler)
    server.health_status = status
    return server


@contextmanager
def running(server):
    server.finish = threading.Event()
    with server:
        worker = threading.Thread(target=server.serve_forever, kwargs={"poll_interval": 0.05})
        worker.start()
        try:
            host, port = server.server_address
            yield f"http://{host}:{port}/health"
        finally:
            server.finish.set()
            server.shutdown()
            worker.join()


class MonitorTests(unittest.TestCase):
    def check(self, expected, *urls):
        result = subprocess.run(
            [sys.executable, str(MONITOR), "--timeout", "0.5", *urls],
            capture_output=True, encoding="utf-8",
            env={**os.environ, "PYTHONUTF8": "1"},
            timeout=10,
        )
        self.assertEqual(result.returncode, int(any(status != "OK" for status in expected)), result.stderr)
        rows = result.stdout.splitlines()
        self.assertEqual([line.split()[0] for line in rows], expected, result.stdout)
        print("\n" + result.stdout.rstrip())
        return result.stdout

    def test_shutdown_services_one_at_a_time(self):
        with running(http_service()) as second_url:
            with running(http_service()) as first_url:
                self.check(["OK", "OK"], first_url, second_url)
            self.check(["REFUSED", "OK"], first_url, second_url)
        self.check(["REFUSED", "REFUSED"], first_url, second_url)

    def test_tcp_connects_but_application_does_not_answer(self):
        server = socketserver.ThreadingTCPServer(("127.0.0.1", 0), SilentHandler)
        with running(server) as url:
            self.check(["RESPONSE_TIMEOUT"], url)

    def test_application_reports_error(self):
        with running(http_service(503)) as url:
            self.assertIn("HTTP 503", self.check(["HTTP_ERROR"], url))


if __name__ == "__main__":
    unittest.main(verbosity=2)
