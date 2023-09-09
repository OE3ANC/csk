# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_VIDEO_CSK6_DVP      video_csk6_dvp.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_GC032A	gc032a.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_GC0328	gc0328.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_SCO30IOT	sco30iot.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV9655	ov9655.c)