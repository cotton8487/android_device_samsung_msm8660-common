import common
import struct

def FullOTA_Assertions(info):
	info.script.AppendExtra("ifelse(!is_mounted(\"/data\"), run_program(\"/sbin/busybox\", \"mount\", \"/data\"));")
	info.script.AppendExtra("ifelse(!is_mounted(\"/system\"), run_program(\"/sbin/busybox\", \"mount\", \"/system\"));")
	info.script.AppendExtra(
		('package_extract_file("install/bin/partitioncheck.sh", "/tmp/partitioncheck.sh");\n'
		'set_metadata("/tmp/partitioncheck.sh", "uid", 0, "gid", 0, "mode", 0777);'))
	info.script.AppendExtra('assert(run_program("/tmp/partitioncheck.sh") == 0);')

def FullOTA_PostValidate(info):
	# run e2fsck
	info.script.AppendExtra('run_program("/sbin/e2fsck", "-fy", "/dev/block/mmcblk0p25");');
	# resize2fs: run and delete
	info.script.AppendExtra('run_program("/tmp/install/bin/resize2fs_static", "/dev/block/mmcblk0p25");');
	# run e2fsck
	info.script.AppendExtra('run_program("/sbin/e2fsck", "-fy", "/dev/block/mmcblk0p25");');

def FullOTA_InstallEnd(info):
	info.script.Mount("/system")
	info.script.AppendExtra('run_program("/sbin/sh", "-c", "busybox echo BOOTIMAGE=/dev/block/mmcblk0p8 > /system/.supersu");')
	info.script.Unmount("/system")
