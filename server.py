#!/usr/bin/python
import random
import re
import select
import socket
import threading
from time import sleep
from uuid import uuid4


class Server(threading.Thread):
    def __init__(self, port):
        threading.Thread.__init__(self)
        self.daemon = True
        self.port = port
        self.srvsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.srvsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.srvsock.bind(("", port))
        self.srvsock.listen(5)
        self.descriptors = [self.srvsock]
        self.players = {}
        self.games = {}
        print('Server started on port %s' % port)

    def run(self):
        while 1:

            # Await an event on a readable socket descriptor
            (sread, swrite, sexc) = select.select(self.descriptors, [], [])

            # Iterate through the tagged read descriptors
            for sock in sread:
                # Received a connect request to the server (listening) socket
                if sock == self.srvsock:
                    self.accept_new_connection()
                else:
                    try:
                        # Received something on a client socket
                        readable_str = False
                        host, port = sock.getpeername()
                        rec = sock.recv(2000)
                        try:
                            strg = rec.decode()
                            readable_str = True
                        except UnicodeDecodeError:
                            strg = rec
                        # Check to see if the peer socket closed
                        if strg == '':
                            self.stop_game(f'{host}:{port}')
                            if f'{host}:{port}' in self.players:
                                del self.players[f'{host}:{port}']
                            strg = f'Client left: {host}:{port}'
                            print(strg)
                            sock.close()
                            self.descriptors.remove(sock)
                        elif readable_str:
                            strg = strg.rstrip()
                            print(f'{host}:{port}: {strg}')
                            m = re.search(r'(READY),([\w\s]*)|(PLAY),([1-9])', strg)
                            if m:
                                if m.group(1):  # READY command
                                    if self.players[f'{host}:{port}']['state'] in ['Player1', 'Player2']:
                                        self.stop_game(f'{host}:{port}')
                                    self.players[f'{host}:{port}']['state'] = 'Ready'
                                    self.players[f'{host}:{port}']['name'] = m.group(2)
                                    self.check_for_pair(f'{host}:{port}')
                                else:  # PLAY received
                                    if self.players[f'{host}:{port}']['state'] in ['Player1', 'Player2']:
                                        self.update_play(f'{host}:{port}', m.group(4))
                                    else:
                                        resp = f'ERROR,{strg},Not paired yet\r\n'
                                        sock.send(resp.encode())
                                pass
                            else:
                                resp = f'ERROR,{strg},Invalid command\r\n'
                                sock.send(resp.encode())

                    except (socket.error, ConnectionResetError):
                        self.stop_game(f'{host}:{port}')
                        if f'{host}:{port}' in self.players:
                            del self.players[f'{host}:{port}']
                        strg = f'Client left: {host}:{port}'
                        print(strg)
                        sock.close()
                        self.descriptors.remove(sock)

    def broadcast_string(self, strg, omit_sock=None):
        for sock in self.descriptors:
            if sock != self.srvsock and sock != omit_sock:
                sock.send(strg)

    def accept_new_connection(self):
        newsock, (remhost, remport) = self.srvsock.accept()
        self.descriptors.append(newsock)
        strg = f'Client joined: {remhost}:{remport}'
        self.players[f'{remhost}:{remport}'] = {'state': 'Connected', 'gameId': None, 'name': '', 'score': 0, 'sock': newsock}
        print(strg)

    def check_for_pair(self, player):
        key1 = player
        key2 = None
        for key in self.players.keys():
            if player != key and self.players[key]['state'] == 'Ready':
                key2 = key
                break
        if key1 and key2:
            k1 = random.randint(0, 1)  # 0 or 1
            k2 = 1 - k1                      # 1 or 0
            self.players[key1]['state'] = f'Player{k1+1}'
            self.players[key2]['state'] = f'Player{k2+1}'
            uuid = str(uuid4())
            self.games[uuid] = {f'player{k1+1}': key1, f'player{k2+1}': key2, 'moves': []}
            self.players[key1]['gameId'] = uuid
            self.players[key2]['gameId'] = uuid
            self.players[key1]['sock'].send(f"START,{k1+1},{self.players[key2]['name']}\r\n".encode())
            self.players[key2]['sock'].send(f"START,{k2+1},{self.players[key1]['name']}\r\n".encode())

    def stop_game(self, player):
        other_player = None
        for game_uuid in self.games.keys():
            if self.games[game_uuid]['player1'] == player:
                other_player = self.games[game_uuid]['player2']
                del self.games[game_uuid]
                break
            elif self.games[game_uuid]['player2'] == player:
                other_player = self.games[game_uuid]['player1']
                del self.games[game_uuid]
                break
        if other_player:
            self.players[player]['gameId'] = None
            self.players[player]['state'] = 'Connected'
            self.players[other_player]['gameId'] = None
            self.players[other_player]['state'] = 'Connected'
            error_str = f'ERROR,Opponent ' + self.players[player]['name'] + ' left\r\n'
            self.players[other_player]['sock'].send(error_str.encode())

    def update_play(self, player, grid_pos):
        new_pos = int(grid_pos)
        other_player = None
        for game_uuid in self.games.keys():
            if self.games[game_uuid]['player1'] == player:
                other_player = self.games[game_uuid]['player2']
                if len(self.games[game_uuid]['moves']) == 9:
                    self.players[player]['sock'].send(f'ERROR,PLAY,{grid_pos},No more move allowed\r\n'.encode())
                    return
                elif len(self.games[game_uuid]['moves']) % 2 == 0:  # Even: player1
                    for pos in self.games[game_uuid]['moves']:
                        if pos == new_pos:
                            self.players[player]['sock'].send(f'ERROR,PLAY,{grid_pos},Spot already taken\r\n'.encode())
                            return
                    self.games[game_uuid]['moves'].append(int(grid_pos))
                    self.players[other_player]['sock'].send(f'PLAY,{grid_pos}\r\n'.encode())
                    return
                else:
                    self.players[player]['sock'].send(f'ERROR,PLAY,{grid_pos},Not your turn\r\n'.encode())
                    return
            elif self.games[game_uuid]['player2'] == player:
                other_player = self.games[game_uuid]['player1']
                if len(self.games[game_uuid]['moves']) == 9:
                    self.players[player]['sock'].send(f'ERROR,PLAY,{grid_pos},No more move allowed\r\n'.encode())
                    return
                elif len(self.games[game_uuid]['moves']) % 2 == 1:  # Odd: player2
                    for pos in self.games[game_uuid]['moves']:
                        if pos == new_pos:
                            self.players[player]['sock'].send(f'ERROR,PLAY,{grid_pos},Spot already taken\r\n'.encode())
                            return
                    self.games[game_uuid]['moves'].append(int(grid_pos))
                    self.players[other_player]['sock'].send(f'PLAY,{grid_pos}\r\n'.encode())
                    return
                else:
                    self.players[player]['sock'].send(f'ERROR,PLAY,{grid_pos},Not your turn\r\n'.encode())
                    return


def main():
    serverport = 10000
    myServer = Server(serverport)
    myServer.start()

    try:
        while 1:
            sleep(1.0)

    except KeyboardInterrupt:
        print('Exiting...')


if __name__ == '__main__':
    main()
