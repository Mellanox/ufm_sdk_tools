import http.client
import socket
import paramiko
import time
import os
import subprocess
import logging
from utils_constants import AccessCreds, ShellCommands
import shutil


def write_to_file(filename, content, hostname, location_on_remote):
    """
    Write content to a file, copy this file to location_on_remote if the hostname is not localhost

    Args:
        filename (str): the filename to create.
        content (str): content of the file.
        hostname (str): device to send it, or to have it.
        location_on_remote (str): location on remote
    """

    try:
        file = open(filename, "w", encoding="utf-8")
        file.write(content)
        file.close()
    except IOError as exception:
        print(f"failed to save {filename} at {location_on_remote}, got: {exception}")
        return
    if not is_localhost(hostname):
        copy_file_to_remote(filename,location_on_remote,hostname)
    else:
        copy_file_to_host(filename,location_on_remote)

def copy_file_to_host(src:str, destination:str) -> None:
    """
    copy a file in src to location on the same machine that running the script

    Args:
        src (str): srouce location
        destination (str): destination location for the file.
    """
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    shutil.copy(src, destination)

def copy_file_to_remote(local_file:str, remote_path:str, server:str, status=False) -> int:
    """copy a local file to remote path, create a folder if not exists

    Args:
        local_file (str): location of file
        remote_path (str): remote path on the server
        server (str): server name
        status (bool, optional): _description_. Defaults to False.

    Returns:
        int: _description_
    """
    if not is_folder_exists(remote_path, server):
        run_command_status(f"mkdir -p {remote_path}", server)
    if ":" in server:
        cmd = f"scp -6 {local_file} root@[{server}]:{remote_path}"
    else:
        cmd = f"scp {local_file} root@{server}:{remote_path}"
        
    return run_command(cmd, verbose=True)

def run_command(command, device=None, verbose=True, timeout=None, no_prefix=True):
    """
    Run Shell command on local or remote device
    Return command exit code.
    """
    if os.environ.get("UFM_REGRISSION_USE_SSH_Docker") and "iperf" not in command:
        if device and not no_prefix:
            command = "docker exec ufm {}".format(command)
    if not is_localhost(device):
        returncode = None
        conn = None
        connection_error = False

        try:
            conn = connect_to_remote(device, AccessCreds.HOST_USER_NAME, '')
        except Exception:
            connection_error = True

        if not conn:
            conn = connect_to_remote(device, AccessCreds.HOST_USER_NAME,
                                        AccessCreds.HOST_PASS)
            connection_error = False
            
        if not conn:
            connection_error = True

        if not connection_error:
            command = command + " 2>&1"
            if timeout:
                cmd = "timeout {0} {1}\n".format(timeout, command)
                channel = ''
                for _ in range(0, 10):
                    _, out, _ = conn.exec_command(cmd, timeout=timeout)
                    channel = out.channel
                    if channel.recv_ready() or channel.recv_stderr_ready() or not channel.closed:
                        break
                    time.sleep(10)
                returncode = channel.recv_exit_status()
            else:
                channel = ''
                for _ in range(0, 10):
                    _, out, _ = conn.exec_command(command)
                    channel = out.channel
                    if channel.recv_ready() or channel.recv_stderr_ready() or not channel.closed:
                        break
                    time.sleep(10)
                returncode = channel.recv_exit_status()
            conn.close()
        else:
            returncode, _, _ = run_ssh_command(command, device, verbose)

        return returncode

    else:
        command = command + " 2>&1"
        if timeout:
            command = f"timeout {timeout} {command}\n"
        with open(os.devnull, 'w',encoding="uft-8") as tempf:
            proc = subprocess.Popen(command, shell=True, stdout=tempf, stderr=tempf)
            proc.communicate()
            return proc.returncode
            
def connect_to_remote(remote_host:str, user:str, passwd:str, docker:bool=False):
    """connect to the remote host using sshClient return connection

    Args:
        remote_host (str): remote device we want to connect to
        user (str): username for connection
        passwd (str): password for connection
        docker (bool, optional): are we connecting to the docker of in that device. Defaults to False.

    Raises:
        e: _description_

    Returns:
        SSHClient: connection to remote host, raise an exception if it was not possible.
    """
    try:
        conn_remote_host = socket.gethostbyname(remote_host)
    except Exception:
        conn_remote_host = remote_host
    for _ in range(0, 5):
        while True:
            try:
                conn = paramiko.SSHClient()
                conn.set_missing_host_key_policy(paramiko.AutoAddPolicy())
                if not docker:
                    conn.connect(conn_remote_host, username=user, password=passwd, banner_timeout=60)
                else:
                    conn.connect(conn_remote_host, username=user, password=passwd, port=2022, banner_timeout=60)
                return conn
            except Exception as e:
                print(e)
                # logging.info("Main SSHClient throws: %s" % e)
                if 'Channel closed' in str(e):
                    time.sleep(1)
                    continue
                else:
                    raise e


def run_command_status(command, device=None, verbose=True, docker=False, no_prefix=True, multi_line_prefix_cmd=False,\
    user=None, password=None, in_container=False, container_name="ufm", under_user=None, docker_option=""):
    """
    Run Shell command on local or remote device
    Return command rd, status, stderr.


    params:
        - under_user - should be set with in_container param;
                        When in_container=True allows to run command under specific user on the host,
                        if not set - will be filled from ENV variable ROOTLESS_USER

    Notes:
        - All commands, starting with "docker" string are running under under_user, when it is set
    """
    if user is None:
        user = AccessCreds.HOST_USER_NAME

    if not under_user:
        under_user = os.environ.get("ROOTLESS_USER")

    if os.environ.get("UFM_REGRISSION_USE_SSH_Docker") and "iperf" not in command or in_container:
        if device and (not no_prefix or in_container):

            if not multi_line_prefix_cmd:
                command = f"docker exec {docker_option} {container_name} {command}"
            else:
                command = f'docker exec {docker_option} {container_name} bash -c "{command}"'

    # currently only for command in_container and in case there is "docker" command
    if under_user and (in_container or command.startswith("docker")):
        command = f"cd /tmp; sudo -u {under_user} {command}"

    if not is_localhost(device):
        returncode, stdout, stderr, conn = (None,) * 4
        connection_error = False

        logging.info("run_command_status (%s) : %s",device,command)
        try:
            conn = connect_to_remote(device, user, '', docker=docker)
        except Exception:
            connection_error = True

        if not conn:
            connection_error = True

        if connection_error:
            try:
                if not password:
                    password = AccessCreds.HOST_PASS
                else:
                    pass
                conn = connect_to_remote(device, user, password)
                connection_error = False
            except Exception as e:
                print(e)
                pass

        if not conn:
            connection_error = True

        if not connection_error:
            channel = ''
            timeout = 130
            for idx in range(0, 10):
                endtime = time.time() + timeout
                start = time.time()
                _, out, err = conn.exec_command(command)
                logging.debug(f"try #{idx +1 }, execution took: {time.time() - start:.3f} sec")
                channel = out.channel

                if channel.recv_ready() or channel.recv_stderr_ready() or not channel.closed:
                    stdout = out.read()
                    stderr = err.read()
                    break
                else:
                    logging.info("Channel Closed")
                    logging.info('recv_ready %r',channel.recv_ready())
                    logging.info('recv_stderr_ready %r',channel.recv_stderr_ready())
                    logging.info('channel.closed %r',channel.closed)
                time.sleep(10)
            returncode = channel.recv_exit_status()
            conn.close()
        else:
            returncode, stdout, stderr = run_ssh_command(
                command, device, verbose)

        out = stdout if isinstance(stdout, str) else stdout.decode()
        err = stderr if isinstance(stderr, str) else stderr.decode()

        logging.info("rc    : %s" , str(returncode))
        logging.info("stdout: %s" , out)
        logging.info("stderr: %s" , err)
        return (returncode, out, err)

    else:
        logging.info("run_command_status(local) : %s", command)
        proc = subprocess.Popen(command, shell=True, close_fds=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = proc.communicate()
        out, err = stdout.decode(), stderr.decode()
        logging.info("rc    : %s", str(proc.returncode))
        logging.info("stdout: %s", out)
        logging.info("stderr: %s", err)
        return (proc.returncode, out, err)


def run_ssh_command(command, device=None, verbose=True):
    """
    Run Shell command on local or remote device
    Return command rd, status, stderr.
    """
    if not is_localhost(device):
        stdout, stderr = None, None
        user_name = AccessCreds.HOST_USER_NAME
        # the shell commands needs those items {user_name}@{device} '{command}'
        if ':' in device:
            command = ShellCommands.SSH_CMD_IPV6.format(user_name,device,command)
        else:
            command = ShellCommands.SSH_CMD.format(user_name,device,command)

    proc = subprocess.Popen(command, shell=True, close_fds=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    out, err = stdout.decode(), stderr.decode()
    return (proc.returncode, out, err)

def is_localhost(device:str):
    """return if the device is localhost
    the device is localhost if it None/empty or it is written inside localhost/127.0.0.1
    Args:
        device (str): _description_

    Returns:
        bool: if the device is localhost or 127.0.0.1
    """
    if device is None or len(device) == 0:
        return True
    return device.lower() == "localhost" or device == "127.0.0.1"

def is_folder_exists(remote_path:str, server:str)->bool:
    """check if the folder exists on the server on the remote_path

    Args:
        remote_path (str): remote path that we want check on the server
        server (str): server device, that is not localhost.

    Returns:
        bool: return if the remote path exists on the server
    """
    try:
        # Initialize SSH client
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        client.connect(server, username=AccessCreds.HOST_PASS, password=AccessCreds.HOST_PASS)

        # Command to check if folder exists
        command = f"[ -d '{remote_path}' ] && echo 'exists' || echo 'not_exists'"
        # we run a echo because we cannot check exit code of a spefic command,
        # it is easier to check if the if was successful
        _, stdout, _ = client.exec_command(command)
        result = stdout.read().decode().strip()

        # Close the SSH connection
        client.close()

        return result == 'exists'
    except Exception as e:
        print(f"Error checking folder existence: {e}")
        return False

def validate_rest_code(code, code_expected=http.client.OK)->None:
    """compare the code and the expected output, Raise an exception if not equal

    Args:
        code (_type_): rest code we want to compare
        code_expected (_type_, optional): expected code we want to see. Defaults to http.client.OK.

    Raises:
        Exception: Exception with invalid response code.
    """
    if code != code_expected:
        raise Exception(f"Invalid response code, found {code}, expected {code_expected}")

    logging.info("Status Code is As Expected")

def kill_proc_by_name(proc_name:str, exclude_proc:str=None, kill_flag:str="-9", device:str=None, raise_exception=True)->int:
    """kill process by name on device, can exclude process, if there is a problem can raise an exception

    Args:
        proc_name (str): name of process we want to kill
        exclude_proc (str, optional): str to exclude when running ps command. Defaults to None.
        kill_flag (str, optional): the kill flag we want to use (default is SIGKILL). Defaults to "-9".
        device (str, optional): on which device we want to kill the process. Defaults to None.
        raise_exception (bool, optional): raise an exception if the pid still there after kill command. Defaults to True. 

    Raises:
        Exception: if there was an issue killing the process.

    Returns:
        int: if kill_flag is -9: 0 is kill was successful, 1 else. otherwise the pids that needs to be killed
    """
    running_proc_ids = []
    proc_ids_cmd = "ps -efw | grep '" + proc_name + "'  | grep -v grep "
    if proc_name == "ufm":
        proc_ids_cmd = "ps -efw | grep '" + proc_name + "'  | grep -v grep | grep -v sql"
    if exclude_proc:
        proc_ids_cmd += " | grep -v " + exclude_proc
    proc_ids_cmd += " | awk '{print $2}'"
    _, stdout, _ = run_command_status(proc_ids_cmd, device)
    for proc_id in stdout.split("\n"):
        if proc_id != '':
            running_proc_ids.append(proc_id)
            run_command("yes | kill " + kill_flag + " " + proc_id, device)
    # Validate proc is down
    if kill_flag == "-9":
        _, stdout, _ = run_command_status(proc_ids_cmd, device)
        if stdout == '':
            logging.info("Success. Process %s isn't running", proc_name)
            return 0
        else:
            if raise_exception:
                raise Exception(f"Error. Process {proc_name}  is running: {stdout}")
            else:
                return 1
    return running_proc_ids
