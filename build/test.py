import subprocess
import time

found_messages = 0

def read_file(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()
    return [line.strip() for line in lines]

def run_client(ip, port, username, channel, messages):
    command = ['./client', '--ip-address', ip, '--port', port]
    global found_messages
    
    try:
        with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True) as process:
            def send_input(data):
                process.stdin.write(data)
                process.stdin.flush()

            def read_output():
                output = process.stdout.readline().strip()
                while output == "":
                    output = process.stdout.readline().strip()
                return output

            # Подключаемся и логинимся
            send_input("2\n")  # Выбираем логин
            send_input("1\n")  # Вводим всегда "1"
            send_input("1\n")  # Вводим пароль
            
            # Ожидание завершения входа
            while True:
                output = read_output()
                if "Press the button:" not in output:
                    break
            
            # Подключение к каналу
            send_input(f"/join {channel}\n")
            while True:
                output = read_output()
                if "connected successfully" in output:
                    break

            # Проверяем логи каждую секунду
            test_started = False
            message_index = 0

            while message_index < len(messages):
                send_input('/read\n')
                responses = [read_output() for _ in range(7)]
                
                for response in responses:
                    print(f"Ответ: {response}")

                    if "TEST STARTED" in response:
                        test_started = True
                        print("Тест начался")

                    if test_started:
                        if message_index < len(messages) and messages[message_index] in response and username in response:
                            print(f"Найдено сообщение: {messages[message_index]}")
                            found_messages += 1
                            message_index += 1

                time.sleep(1)

            # Завершаем работу с программой
            send_input('/exit\n')
            process.wait()

    except subprocess.CalledProcessError as e:
        print(f"Произошла ошибка при выполнении команды: {e}")
    except Exception as e:
        print(f"Неожиданная ошибка: {e}")

if __name__ == "__main__":
    file_path = 'messages.txt'
    lines = read_file(file_path)

    ip_port, username, channel = lines[0].split()
    ip, port = ip_port.split(':')
    test_messages = lines[2:]

    run_client(ip, port, username, channel, test_messages)

    if len(test_messages) == found_messages:
        print("SUCCESS :D")
    else:
        print("FAILED D:")