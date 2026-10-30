import os.path
import requests
import zipfile
import platform

url = r"https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-devel-2.30.7-VC.zip"

def url_download(url):
    file_name = os.path.split(url)[-1]
    print(file_name)
    if os.path.isfile(file_name):
        return file_name

    with open(file_name, 'wb') as file:
        response = requests.get(url)
        file.write(response.content)
    return file_name

def unzip_file(zip_file_path, extract_dir):
    with zipfile.ZipFile(zip_file_path, 'r') as zip_ref:
        zip_ref.extractall(extract_dir)

extract_path = r'E:\SDL2'

unzip_file(url_download(url), extract_path)

bits = platform.architecture()[0]

if bits == '64bit':
    for root, dirs, files in os.walk(extract_path):
        for dir in dirs:
            if dir == 'cmake':
                print(os.path.join(root, dir))
            if dir == 'x64':
                print(os.path.join(root, dir))
elif bits == '32bit':
    for root, dirs, files in os.walk(extract_path):
        for dir in dirs:
            if dir == 'cmake':
                print(os.path.join(root, dir))
            if dir == 'x86':
                print(os.path.join(root, dir))
