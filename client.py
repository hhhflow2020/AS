import asyncio
import struct

async def send_message(reader, writer, user_id, pack_id, message):
    message = message.encode('utf-8')
    message_length = len(message)
    header = struct.pack('!LLQ', message_length, pack_id, user_id)
    writer.write(header + message)
    await writer.drain()

    response_header = await reader.readexactly(16)
    response_length, response_pack_id, response_user_id = struct.unpack('!LLQ', response_header)
    response_body = await reader.readexactly(response_length)

    response = response_body.decode('utf-8')
    print(f'Received response for pack_id={response_pack_id}, user_id={response_user_id}: {response}')

async def main():
    try:
        reader, writer = await asyncio.open_connection('127.0.0.1', 7000)
    except (ConnectionError, OSError) as e:
        print(f'Failed to connect: {e}')
        return
    
    user_id = 12345678
    pack_id = 1
    message = 'Hello, world!'
    await send_message(reader, writer, user_id, pack_id, message)

    writer.close()
    await writer.wait_closed()

if __name__ == '__main__':
    asyncio.run(main())
