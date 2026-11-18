"""Проверка доступности HTTP-сервисов по указанным URL."""

import argparse
import http.client
import math
from urllib.parse import urlsplit


def service_url(value):
    try:
        url = urlsplit(value)
        port = url.port
    except ValueError as error:
        raise argparse.ArgumentTypeError(f"Некорректный URL: {value}") from error

    if (
        url.scheme not in {"http", "https"}
        or not url.hostname
        or port == 0
        or url.username is not None
        or url.fragment
        or any(not 33 <= ord(char) <= 126 for char in value)
    ):
        raise argparse.ArgumentTypeError(f"Укажи HTTP(S) URL, например http://127.0.0.1:8000/health: {value}")
    return url


def positive_timeout(value):
    try:
        seconds = float(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("Тайм-аут должен быть числом") from error
    if not math.isfinite(seconds) or seconds <= 0:
        raise argparse.ArgumentTypeError("Тайм-аут должен быть конечным числом больше нуля")
    return seconds


def check_service(url, timeout):
    connection_type = http.client.HTTPSConnection if url.scheme == "https" else http.client.HTTPConnection
    connection = connection_type(url.hostname, url.port, timeout=timeout)
    path = url.path or "/"
    if url.query:
        path += "?" + url.query

    try:
        try:
            connection.connect()
        except ConnectionRefusedError:
            return "REFUSED", "Соединение отклонено"
        except TimeoutError:
            return "CONNECT_TIMEOUT", "Истекло время подключения"
        except (OSError, OverflowError) as error:
            return "CONNECT_ERROR", str(error)

        try:
            connection.request("GET", path)
            response = connection.getresponse()
        except TimeoutError:
            return "RESPONSE_TIMEOUT", "Соединение открыто, HTTP-ответ не получен вовремя"
        except http.client.RemoteDisconnected:
            return "NO_RESPONSE", "Сервис закрыл соединение без HTTP-ответа"
        except (OSError, http.client.HTTPException) as error:
            return "RESPONSE_ERROR", str(error)

        status = response.status
        if 200 <= status < 300:
            return "OK", f"HTTP {status}"
        return "HTTP_ERROR", f"HTTP {status}"
    finally:
        connection.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("urls", nargs="+", type=service_url, metavar="URL")
    parser.add_argument("--timeout", type=positive_timeout, default=2.0, help="тайм-аут сетевой операции, с")
    args = parser.parse_args()

    failed = False
    for url in args.urls:
        status, detail = check_service(url, args.timeout)
        print(f"{status:16} {url.geturl()}  {detail}")
        failed |= status != "OK"
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
