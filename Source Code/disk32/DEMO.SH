# SINO SH demonstration. Created files live in RAM and reset on reboot.
uname -a
pwd
ls /boot
mkdir -p /tmp/demo
echo "hello from SINO SH" > /tmp/demo/hello.txt
echo "RAM files are temporary" >> /tmp/demo/hello.txt
cat /tmp/demo/hello.txt
cat /tmp/demo/hello.txt | grep hello
wc /tmp/demo/hello.txt
run /boot/hello.f32
df
