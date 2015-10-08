#include <dirent.h> 
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio> 
#include <cstdlib>
#include <string>


#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <linux/usb/video.h>

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

      int fd = open(("/dev/"+name).c_str(), 0); // Or possibly O_NONBLOCK
      ioctl(fd, VIDIOC_STREAMON, NULL);

      char buffer[640*480*4];
      size_t num = read(fd, buffer, 640*480*2);
      printf("Read: %d B\n", num);

      ioctl(fd, VIDIOC_STREAMOFF, NULL);
      close(fd);
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
