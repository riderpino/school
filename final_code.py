import random
import telnetlib
import time
import os 
from subprocess import call

import iperf3
iperf3.IPerf3.__del__ = lambda self: None 
from datetime import datetime
import json



def generate_vlan_config():
    class cable_conn:
        def __init__(self, A, B):
            self.A = A
            self.B = B

    class vlan_conn:
        def __init__(self, A, B):
            self.A = A
            self.B = B

    class last_vlan:
        def __init__(self, A, B, C, D, E):
            self.A = A
            self.B = B
            self.C = C
            self.D = D
            self.E = E


    max_vlan_number = 8
    vlan_counter = 0

    cable_conn_list_banco_A = [cable_conn(A, B) for A, B in [(3,5), (7,9), (11,13), (15,17), (19,21)]]
    cable_conn_list_banco_B = [cable_conn(A, B) for A, B in [(4,6), (8,10), (12,14), (16,18), (20,22)]]

    print(cable_conn_list_banco_A[2].B)

    for connection in cable_conn_list_banco_A: 
        print(connection.A, " ", connection.B)

    print("number of item in banco_b cable connection is: ", len(cable_conn_list_banco_B))

    #qui prendo il random
    random_banco_b = random.randint(0, len(cable_conn_list_banco_B)-1)


    vlan_connection_list = []




    a_vlan = 1
    #con questo, scelgo a caso tra A o B di una connessione scelta a caso dalla lista banco b
    a_t = cable_conn_list_banco_B[random_banco_b].A
    b_t = cable_conn_list_banco_B[random_banco_b].B
    b_vlan = random.choice([a_t,b_t])
    vlan_1 = vlan_conn(a_vlan,b_vlan)
    print( "vlan è ", vlan_1.A, vlan_1.B)
    cable_conn_list_banco_B.pop(random_banco_b)

    vlan_connection_list.append(vlan_1)
    is_B = True

    while( vlan_counter < max_vlan_number-2):
        #print(len(cable_conn_list_banco_B))
        #print(len(cable_conn_list_banco_A))
        if len(cable_conn_list_banco_A) == 0:
            print("No more items left to select!")
            break  # Exit the loop
        if len(cable_conn_list_banco_B) == 0:
            print("No more items left to select!")
            break  # Exit the loop
        random_banco_b = random.randint(0, len(cable_conn_list_banco_B)-1)
        random_banco_a = random.randint(0, len(cable_conn_list_banco_A)-1)
        

        if b_vlan == b_t : #allora so che il nodo b che ho nella vlan corrispone al nodo b che ho nella connessione del cavo
            a_vlan = a_t #allora il nodo A della nuova vlan che creo deve essere il vecchio -1 A del cavo connessione A -1
        else: 
            a_vlan = b_t #se B_vlan -1 era A allora inserisco Il vecchio -1 B del cavo connessio B -1

            #ho bisogno di alternare Banco a a banco B

            #se il vecchio banco era B allora fai Banco A e viceversa
        if is_B:
            a_t = cable_conn_list_banco_B[random_banco_b].A
            b_t = cable_conn_list_banco_B[random_banco_b].B
            b_vlan = random.choice([a_t,b_t])
            vlan_n = vlan_conn(a_vlan,b_vlan)
            is_B = False
            cable_conn_list_banco_B.pop(random_banco_b)

        else :
            a_t = cable_conn_list_banco_A[random_banco_a].A
            b_t = cable_conn_list_banco_A[random_banco_a].B
            b_vlan = random.choice([a_t,b_t])
            vlan_n = vlan_conn(a_vlan,b_vlan)
            is_B = True
            cable_conn_list_banco_A.pop(random_banco_a)

        


        vlan_connection_list.append(vlan_n)
        #print( "vlan è ", vlan_n.A, vlan_n.B)
        

        
        
        vlan_counter = vlan_counter +1 
        ###################################
        

    if b_vlan == b_t : #allora so che il nodo b che ho nella vlan corrispone al nodo b che ho nella connessione del cavo
        a_vlan = a_t #allora il nodo A della nuova vlan che creo deve essere il vecchio -1 A del cavo connessione A -1
    else: 
        a_vlan = b_t #se B_vlan -1 era A allora inserisco Il vecchio -1 B del cavo connessio B -1

    the_last_vlan = last_vlan(a_vlan, 23, 24, 25, 26) #last connection vlan is bigger 
    vlan_connection_list.append(the_last_vlan)


    for n in vlan_connection_list:
        if isinstance(n, last_vlan):
            print("the last vlan is:", n.A, n.B, n.C, n.D, n.E)
        else:
            print("the vlan is:", n.A, n.B)   
    return vlan_connection_list






def write_on_the_switch(vlan_list): 
    HOST = "192.168.1.213"
    USERNAME = "admin"
    PASSWORD = "manager"
    sleeptime = 2
    
    try:
        tn = telnetlib.Telnet(HOST)
        tn.set_debuglevel(sleeptime) 

        #the start
        tn.read_until(b"Press any key to continue", timeout=10)
        tn.write(b"\n")
        time.sleep(sleeptime)

        # Login
        tn.read_until(b"Username: ", timeout=5)
        tn.write(USERNAME.encode("ascii") + b"\n")
        tn.read_until(b"Password: ", timeout=5)
        tn.write(PASSWORD.encode("ascii") + b"\n")
        time.sleep(sleeptime)

        # Configurazione VLAN 
        tn.write(b"configure terminal\n")
        time.sleep(sleeptime)

        ##sososo nothing was working beacuse the thing here is that: 

        #first i deactivate all the port between 2 and 22
        tn.write(b"interface 2-22\n")
        time.sleep(sleeptime)
        tn.write(b"disable\n")
        time.sleep(sleeptime)

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(b"untagged 1-26\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)





        A = vlan_list[0].A
        B = vlan_list[0].B
        PORT_LIST = f"{A},{B}"

        tn.write(b"vlan 2\n")
        time.sleep(sleeptime)
        tn.write(b"no untagged all\n")
        time.sleep(sleeptime)
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(f"no untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)



       






        A = vlan_list[1].A
        B = vlan_list[1].B
        PORT_LIST = f"{A},{B}"

        tn.write(b"vlan 3\n")
        time.sleep(sleeptime)
        tn.write(b"no untagged all\n")
        time.sleep(sleeptime)
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(f"no untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)

        
        

        A = vlan_list[2].A
        B = vlan_list[2].B
        PORT_LIST = f"{A},{B}"

        tn.write(b"vlan 4\n")
        time.sleep(sleeptime)
        tn.write(b"no untagged all\n")
        time.sleep(sleeptime)
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(f"no untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)
       

        A = vlan_list[3].A
        B = vlan_list[3].B
        PORT_LIST = f"{A},{B}"

        tn.write(b"vlan 5\n")
        time.sleep(sleeptime)
        tn.write(b"no untagged all\n")
        time.sleep(sleeptime)
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(f"no untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)

       

        A = vlan_list[4].A
        B = vlan_list[4].B
        PORT_LIST = f"{A},{B}"

        tn.write(b"vlan 6\n")
        time.sleep(sleeptime)
        tn.write(b"no untagged all\n")
        time.sleep(sleeptime)
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(f"no untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)

       

        A = vlan_list[5].A
        B = vlan_list[5].B
        PORT_LIST = f"{A},{B}"

        tn.write(b"vlan 7\n")
        time.sleep(sleeptime)
        tn.write(b"no untagged all\n")
        time.sleep(sleeptime)
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(f"no untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)

       

        A = vlan_list[6].A
        B = vlan_list[6].B
        PORT_LIST = f"{A},{B}"

        tn.write(b"vlan 8\n")
        time.sleep(sleeptime)
        tn.write(b"no untagged all\n")
        time.sleep(sleeptime)
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        tn.write(f"no untagged {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)

        


        A = vlan_list[7].A
        B = 23
        C = 24
        D = 25
        E = 26
        PORT_LIST = f"{A},{B},{C},{D},{E}"

        tn.write(b"vlan 1\n")
        time.sleep(sleeptime)
        #tn.write(b"no untagged 1-22\n")
        tn.write(f"untagged {PORT_LIST}\n".encode("ascii"))
        tn.write(b"write memory\n")

        
        
        time.sleep(sleeptime)
        tn.write(f"interface {PORT_LIST}\n".encode("ascii"))
        time.sleep(sleeptime)
        tn.write(b"enable\n")
        time.sleep(sleeptime)
        tn.write(b"write memory\n")
        time.sleep(sleeptime)
        tn.write(b"exit\n")
        time.sleep(sleeptime)

        time.sleep(sleeptime)
        print("sono quiiii")

        tn.write(b"write memory\n")
        tn.write(b"exit\n")
        tn.write(b"exit\n")
        tn.read_until(b"#", timeout=5)
        time.sleep(sleeptime)
        
        tn.close()
    except Exception as e:
        print(f"non va nullaaa: {e}")






def log_results(vlan_list, result):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    log_entry = f"\n==== {timestamp} ====\n"

    log_entry += "Current VLAN configuration:\n"
    for vlan in vlan_list:
        if hasattr(vlan, 'C'):  # è la last_vlan
            ports = f"{vlan.A},{vlan.B},{vlan.C},{vlan.D},{vlan.E}"
            log_entry += f"DEFAULT_VLAN → Ports: {ports}\n"
        else:
            ports = f"{vlan.A},{vlan.B}"
            log_entry += f"VLAN → Ports: {ports}\n"

    if result and not result.error:
        try:
            end = result.json['end']
            sender = end['sum_sent']
            receiver = end['sum_received']

            log_entry += "\niperf3 Summary (7200s TCP test):\n"
            log_entry += f"  Total Bytes Sent:     {sender['bytes'] / 1e6:.2f} MB\n"
            log_entry += f"  Total Bytes Received: {receiver['bytes'] / 1e6:.2f} MB\n"
            log_entry += f"  Average Sending Rate: {sender['bits_per_second'] / 1e6:.2f} Mbps\n"
            log_entry += f"  Average Receiving Rate: {receiver['bits_per_second'] / 1e6:.2f} Mbps\n"
            log_entry += f"  Total Retransmissions: {sender.get('retransmits', '?')}\n"

            log_entry += f"\nRTT Stats (Sender):\n"
            log_entry += f"  Min RTT:  {sender.get('min_rtt', '?')} ms\n"
            log_entry += f"  Mean RTT: {sender.get('mean_rtt', '?')} ms\n"
            log_entry += f"  Max RTT:  {sender.get('max_rtt', '?')} ms\n"

            log_entry += f"\nTCP Congestion Control:\n"
            log_entry += f"  Sender:   {end.get('sender_tcp_congestion', '?')}\n"
            log_entry += f"  Receiver: {end.get('receiver_tcp_congestion', '?')}\n"

            log_entry += f"\nWindowing:\n"
            log_entry += f"  Max congestion window (cwnd): {sender.get('max_snd_cwnd', '?')} bytes\n"

            if 'cpu_utilization_percent' in end:
                cpu = end['cpu_utilization_percent']
                log_entry += f"\nCPU Utilization:\n"
                log_entry += f"  Sender:   total {cpu.get('host_total', '?'):.2f}% (user {cpu.get('host_user', '?'):.2f}%, sys {cpu.get('host_system', '?'):.2f}%)\n"
                log_entry += f"  Receiver: total {cpu.get('remote_total', '?'):.2f}% (user {cpu.get('remote_user', '?'):.2f}%, sys {cpu.get('remote_system', '?'):.2f}%)\n"

            # Get last stream metrics for final RTT, cwnd, retrans
            try:
                last_stream = result.json['intervals'][-1]['streams'][0]
                cwnd_kb = last_stream['snd_cwnd'] / 1024
                rtt = last_stream['rtt'] / 1000
                log_entry += f"\nFinal Transmission Metrics:\n"
                log_entry += f"  Final cwnd:  {cwnd_kb:.1f} KB\n"
                log_entry += f"  Final RTT:   {rtt:.2f} ms\n"
                log_entry += f"  Final retransmits (1s): {last_stream.get('retransmits', '?')}\n"
                if 'pacing_rate' in last_stream:
                    pacing = last_stream['pacing_rate'] / 1e6
                    log_entry += f"  Pacing Rate: {pacing:.2f} Mbps\n"
            except Exception as e:
                log_entry += f"\n[Warning] Could not extract final stream metrics: {e}\n"


        except Exception as e:
            log_entry += f"\nError parsing result JSON: {e}\n"
    else:
        log_entry += f"\niperf3 Error: {result.error if result else 'No result'}\n"

    log_entry += "\n========================\n"

    with open("iperf3_vlan_log.txt", "a") as log_file:
        log_file.write(log_entry)


def is_host_up(ip):
    return call(["ping", "-c", "1", "-W", "2", ip], stdout=open(os.devnull, 'w')) == 0

def running_code(): 
    


    while True:
        
        ip_address = '192.168.1.191'
        vlans = generate_vlan_config()

        print (vlans[2].A)

        write_on_the_switch(vlans)
        if not is_host_up(ip_address):
            print(f"⚠️ Host {ip_address} unreachable. Skipping iperf3.")
            time.sleep(10)
            continue
        
        try:
            client = iperf3.Client()
            client.duration = 7200
            client.server_hostname = ip_address
            client.port = 5201
            client.protocol = 'tcp' 
            client.json_output = True
            #print(f'Connecting to {client.server_hostname}:{client.port}')
            result = client.run()
        except Exception as e:
            result = None
            print(f"iperf3 client error: {e}")
        

        if result is not None and result.error:
            print("aia")
            
        elif result is not None:
            print('\n test completed:')

        else:
            print('no result from iperf3')
        
        log_results(vlans, result)
        print(" waiting 10 seconds before next cycle... .. ..\n")    
        time.sleep(15)



running_code()
    


