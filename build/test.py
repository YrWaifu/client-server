import subprocess
import time
import threading

def read_output(process, output):
    while True:
        line = process.stdout.readline()
        if line:
            output.append(line.strip())
        else:
            break

command = ['./client', '--ip-address', '127.0.0.1', '--port', '8080']

process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1, text=True)

# Start a thread to read stdout
output = []
thread = threading.Thread(target=read_output, args=(process, output))
thread.start()

process.stdin.write('/join channel3\n')
process.stdin.flush()

try:
    for i in range(5):
        process.stdin.write('asd\n')
        process.stdin.flush()
        print(output)
        time.sleep(1)
except KeyboardInterrupt:
    process.terminate()
    process.wait()

process.stdin.write('/exit\n')
process.stdin.flush()

# Wait for the reader thread to finish
thread.join()

print(output)


# import subprocess
# import time
# import select

# command = ['./client', '--ip-address', '127.0.0.1', '--port', '8080']

# process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1, text=True)

# process.stdin.write('/join channel3\n')
# process.stdin.flush()

# output = []

# try:
#     for i in range(5):
#         process.stdin.write('asd\n')
#         process.stdin.flush()
        
#         ready, _, _ = select.select([process.stdout], [], [], 1)
#         if ready:
#             output.append(process.stdout.readline().strip())
        
#         time.sleep(1)
# except KeyboardInterrupt:
#     process.terminate()
#     process.wait()

# process.stdin.write('/exit\n')
# process.stdin.flush()

# print(output)