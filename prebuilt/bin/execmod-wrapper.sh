#!/system/bin/sh

if [ "${0}" = "/system/bin/netmgrd" ]; then
	exec "/system/bin/netmgrd.exec" "${@}"
elif [ "${0}" = "/system/bin/qcks" ]; then
	exec "/system/bin/qcks.exec" "${@}"
elif [ "${0}" = "/system/bin/qmiproxy" ]; then
	exec "/system/bin/qmiproxy.exec" "${@}"
elif [ "${0}" = "/system/bin/qmuxd" ]; then
	exec "/system/bin/qmuxd.exec" "${@}"
elif [ "${0}" = "/system/bin/rmt_storage" ]; then
	exec "/system/bin/rmt_storage.exec" "${@}"
fi

exit 1
