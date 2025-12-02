#!/usr/bin/env python3
import argparse
import json
import sys
from dataclasses import dataclass
from typing import Optional


CRC16_INIT = 0xFFFF
CRC16_POLY = 0x1021


@dataclass
class Frame:
    frame_type: str
    length: int
    seq: int
    payload: str
    crc: int
    raw_body: str
    cmd: Optional[str] = None
    ack: Optional[str] = None
    code: Optional[int] = None


def crc16_ccitt(data: bytes) -> int:
    crc = CRC16_INIT
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ CRC16_POLY) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def build_request(seq: int, cmd: str, payload: str) -> str:
    body = f"{seq}|{cmd}|{payload}"
    crc = crc16_ccitt(body.encode("ascii"))
    return f"#{len(body):03d}|{body}|{crc:04X}"


def build_response(seq: int, ack: str, code: int, payload: str) -> str:
    body = f"{seq}|{ack}|{code}|{payload}"
    crc = crc16_ccitt(body.encode("ascii"))
    return f"#{len(body):03d}|{body}|{crc:04X}"


def parse_frame(frame_text: str) -> Frame:
    text = frame_text.strip()
    if not text.startswith("#"):
        raise ValueError("frame must start with '#'")

    first_sep = text.find("|")
    if first_sep <= 1:
        raise ValueError("missing length/body separator")

    length_text = text[1:first_sep]
    try:
        declared_length = int(length_text, 10)
    except ValueError as exc:
        raise ValueError("invalid length field") from exc

    body_and_crc = text[first_sep + 1 :]
    last_sep = body_and_crc.rfind("|")
    if last_sep <= 0:
        raise ValueError("missing crc separator")

    body = body_and_crc[:last_sep]
    crc_text = body_and_crc[last_sep + 1 :]
    try:
        received_crc = int(crc_text, 16)
    except ValueError as exc:
        raise ValueError("invalid crc field") from exc

    if declared_length != len(body):
        raise ValueError(f"length mismatch: declared={declared_length} actual={len(body)}")

    computed_crc = crc16_ccitt(body.encode("ascii"))
    if received_crc != computed_crc:
        raise ValueError(f"crc mismatch: received={received_crc:04X} computed={computed_crc:04X}")

    parts = body.split("|")
    if len(parts) == 3:
        seq_text, cmd, payload = parts
        return Frame(
            frame_type="request",
            length=declared_length,
            seq=int(seq_text, 10),
            cmd=cmd,
            payload=payload,
            crc=received_crc,
            raw_body=body,
        )

    if len(parts) == 4 and parts[1] in {"ACK", "NACK"}:
        seq_text, ack, code_text, payload = parts
        return Frame(
            frame_type="response",
            length=declared_length,
            seq=int(seq_text, 10),
            ack=ack,
            code=int(code_text, 10),
            payload=payload,
            crc=received_crc,
            raw_body=body,
        )

    raise ValueError("unsupported frame shape")


def frame_to_json(frame: Frame) -> str:
    obj = {
        "type": frame.frame_type,
        "length": frame.length,
        "seq": frame.seq,
        "payload": frame.payload,
        "crc": f"{frame.crc:04X}",
        "raw_body": frame.raw_body,
    }
    if frame.cmd is not None:
        obj["cmd"] = frame.cmd
    if frame.ack is not None:
        obj["ack"] = frame.ack
    if frame.code is not None:
        obj["code"] = frame.code
    return json.dumps(obj, ensure_ascii=True, sort_keys=True)


def send_frame(port: str, baud: int, timeout_s: float, frame_text: str, json_mode: bool) -> int:
    try:
        import serial  # type: ignore
    except ImportError:
        print("error: pyserial is required for send mode", file=sys.stderr)
        return 2

    with serial.Serial(port, baudrate=baud, timeout=timeout_s) as ser:
        ser.reset_input_buffer()
        ser.write((frame_text + "\r\n").encode("ascii"))
        ser.flush()
        response = ser.readline().decode("ascii", errors="replace").strip()

    if json_mode:
        obj = {
            "request": frame_text,
            "response": response,
        }
        if response:
            try:
                obj["response_frame"] = json.loads(frame_to_json(parse_frame(response)))
            except Exception as exc:  # pragma: no cover
                obj["response_error"] = str(exc)
        print(json.dumps(obj, ensure_ascii=True, sort_keys=True))
    else:
        print(frame_text)
        if response:
            print(response)
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="OV-Watch BLE framed protocol helper")
    sub = parser.add_subparsers(dest="subcmd", required=True)

    p_req = sub.add_parser("encode-request", help="build a request frame")
    p_req.add_argument("--seq", type=int, required=True)
    p_req.add_argument("--command", required=True)
    p_req.add_argument("--payload", default="")

    p_resp = sub.add_parser("encode-response", help="build a response frame")
    p_resp.add_argument("--seq", type=int, required=True)
    p_resp.add_argument("--ack", choices=["ACK", "NACK"], required=True)
    p_resp.add_argument("--code", type=int, required=True)
    p_resp.add_argument("--payload", default="")

    p_decode = sub.add_parser("decode", help="decode a frame into JSON")
    p_decode.add_argument("frame")

    p_send = sub.add_parser("send", help="send a request frame over serial and print one response line")
    p_send.add_argument("--port", required=True)
    p_send.add_argument("--baud", type=int, default=115200)
    p_send.add_argument("--timeout-s", type=float, default=2.0)
    p_send.add_argument("--seq", type=int, required=True)
    p_send.add_argument("--command", required=True)
    p_send.add_argument("--payload", default="")
    p_send.add_argument("--json", action="store_true")

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        if args.subcmd == "encode-request":
            print(build_request(args.seq, args.command, args.payload))
            return 0

        if args.subcmd == "encode-response":
            print(build_response(args.seq, args.ack, args.code, args.payload))
            return 0

        if args.subcmd == "decode":
            print(frame_to_json(parse_frame(args.frame)))
            return 0

        if args.subcmd == "send":
            frame_text = build_request(args.seq, args.command, args.payload)
            return send_frame(args.port, args.baud, args.timeout_s, frame_text, args.json)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    print("error: unknown command", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
