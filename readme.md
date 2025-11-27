# libVARF
Stands for Virtual Archive Resources Files.  
An all-in-one c++ stream based library for managing directories, files, file archival and resource management.  

A multiplatform file utility library.

## Files and directories
The file and directories part of this library is inspired by ImGuis syntax    
You can change working directory with Push, this will append to the current working directory, and can go back with Pop, you can't pop further than the current root, the root is usually the starting working directory (where the program is being executed)  
**Example 1: open file**
```c++
// appends a folder to the current working directory
// also modifies the current working directory
if (varf::Push("path"))
{
	// returns an fstream to .../path/foo.txt in read mode
	// if file not found in read mode returns nullptr
	auto fstream_ptr = varf::PushFile("foo.txt", varf::mode::read);
	
	// works similar
	std::fstream stream("foo.txt", std::ios::in);
	// returns to previous working directory
	varf::Pop();
}
```

**Example 2: get all files in a path**
```c++
if (varf::Push("path"))
{
	// returns all file paths in current path and all its
	// subdirectories that are either jpg or png
	auto files = varf::Traverse({
		.mode    = varf::files, 
		.depth   = varf::TRAVERSE_FULL,
		.filters = {".png", ".jpg"}
	});

	varf::Pop();
}
```

**Example 3: change path to a known path**
```c++
// sets the working path and varf root to
// the systems appdata directory, in windows
// it would be %APPDATA%
// and in linux is set to ~
varf::SetRootToKnownPath("APPDATA");

if (varf::Push("path"))
{
	// ...
	varf::Pop();
}
```

**Example 4: slurp file**
```c++
if (varf::Push("path"))
{
	// slurp file as a string
	std::sting data_str = varf::slurp("foo.txt");
	// slurp file as a vector
	auto data_vec = varf::slurp<vector<uint8_t>>("foo.txt");

	// same as varf::PushFile("foo.txt", varf::mode::read);
	std::ifstream stream("foo.txt");
	// slurp data from a stream current pos to eof
	auto data_from_stream = varf::slurp(stream);

	varf::Pop();
}
```

## Resourcess
There are two modes, normal mode, and embedding, to set embedding mode on you have to set VARF_EMBED_RESOURCES On in Cmake, this will embed the resource folder into the program.

**Example: using resources**
```c++
// obtains a shared_ptr to an istream
auto stream_ptr = varf::rcs::Get("resources/foo.txt");
// obtains a resource as a vector
std::string resource = varf::rcs::Slurp("resources/foo.txt");
```

## Archive
Currently only limited zip and rezip are supported, it allows the user to store raw data obtained from streams into an archive of the user choosing  
**Example 1: creating zip archive from stream**
```c++
std::istream stream = ...;
// tries to create a zip archive from a stream
// will throw if this fails
varf::ZipArchive archive(stream);

// obtains a list of all the entries in the archive
auto files = archive.GetDirectory();
for(const auto& file : files)
{
	// obtains data from the archive without removing it
	auto data = archive.Peek(file);
	// removes an entry from the archive
	archive.Pop(file);
}
```

**Example 2: adding data to a zip archive and writing it**
```c++
// creates empty archive
varf::ZipArchive archive;

// obtains a list of all the entries in the archive
if (varf::Push("some/path"))
{
	auto stream = varf::PushFile("foo.txt", varf::mode::read);
	// adds stream data to the archive as some/file
	archive.Push("idk/foo.txt", *stream);

	varf::Pop();
}

if (varf::Push("other/path"))
{
	auto stream = varf::PushFile("bar.txt", varf::mode::write);
	// adds stream data to the archive as some/file
	archive.Write(*stream);

	varf::Pop();
}

```

To use Rezip just use RezipArchive instead of ZipArchive, resources embedding uses Rezip.

## Virtual File System
This library contains a simple virtual file system, you can add files from an archive, directory, or add raw data as files, virtual files can be obtained or removed  
**Example: vfs usage**
```c++
// creates a virtual file system
auto vfs = varf::vfs::Create();
if (varf::Push("path"))
{
	// loads from current path
	vfs.Load();
	varf::Pop(varf::POP_FULL);
}

// removes foo.txt from the virtal file system
vfs.Remove("foo.txt");

std::vector<uint8_t> data = ...
// adds raw data as bar.txt to the virutal file system
vfs.Add("bar.txt", data);

if (vfs.Contains("bar.txt"))
{
	// returns a shared ptr to an istream
	auto stream = vfs.Get("bar.txt");
}
```