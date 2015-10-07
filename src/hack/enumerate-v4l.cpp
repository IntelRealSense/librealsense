#include <dirent.h> 
#include <cstdio> 
#include <cstdlib>
#include <string>

void check_device(const std::string & name)
{
  std::string path = "/sys/class/video4linux/" + name + "/device/input";
  DIR *d;
  struct dirent *dir;
  d = opendir(path.c_str());
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      std::string name2 = dir->d_name;
      if(name2.size() < 5 || name2.substr(0,5) != "input") continue;
      std::string cmd = "cat " + path + "/" + name2 + "/id/vendor";
      system(cmd.c_str());
      cmd = "cat " + path + "/" + name2 + "/id/product";
      system(cmd.c_str());
    }

    closedir(d);
  }
}

int main(void)
{
  DIR *d;
  struct dirent *dir;
  d = opendir("/sys/class/video4linux");
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      std::string name = dir->d_name;
      if(name == "." || name == "..") continue;
      printf("%s:\n", name.c_str());
      check_device(name);
    }

    closedir(d);
  }

  return(0);
}
