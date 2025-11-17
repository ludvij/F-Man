import shutil
from sys import argv



def main():
	if len(argv) < 2:
		print(f'Usage: {argv[0]} path/to/folder')
		return
	
	file_name = argv[1]
	shutil.make_archive('out', 'zip', file_name)

if __name__ == '__main__':
	main()