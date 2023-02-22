#

# 使用python asyncio编写tcp服务端，要求: 监听7000端口, 设置端口可重用, 新建连接的时候打印新连接信息，断开连接的时候也打印断开时的信息，连接设置tcp nodelay选项, 使用包头为: 4字节做包体长度, 4字节做pack_id, 8字节做user_id。


import asyncio
import struct
import datetime
import yaml
import logging

async def handle_client(reader, writer):
    peername = writer.get_extra_info('peername')
    print(f'New connection from {peername}')
    # socket = writer.get_extra_info('socket')
    # socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    try:
        while True:
            # 读取包头
            header = await reader.readexactly(16)
            body_length, pack_id, user_id = struct.unpack('!LLQ', header)

            # 读取包体
            body = await reader.readexactly(body_length)

            print(f'Received {len(header) + len(body)} bytes from {peername}: pack_id={pack_id}, user_id={user_id}, body={body}')

            # 回复响应
            response = b'OK'
            response_header = struct.pack('!LLQ', len(response), pack_id, user_id)
            writer.write(response_header + response)
            await writer.drain()
    except asyncio.exceptions.IncompleteReadError:
        pass
    finally:
        # 连接关闭时打印当前时间
        print(f'Connection from {peername} closed at {datetime.datetime.now()}')
        writer.close()

async def main():
    server = await asyncio.start_server(handle_client, '127.0.0.1', 7000, reuse_port=True)


    print('Server started')

    async with server:
        await server.serve_forever()

if __name__ == '__main__':
    asyncio.run(main())
