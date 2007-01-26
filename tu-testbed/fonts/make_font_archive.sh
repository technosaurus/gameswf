#!/bin/bash

# Package up font files w/ LICENSE.txt

datestamp=`date +%Y%m%d`
filename="tuffy-$datestamp.tar.gz"

# A sed pattern used by tar to prepend a path to each filename.
transform_pattern=s:^:tuffy-$datestamp/:

tar czvf $filename LICENSE.txt --transform="$transform_pattern" tuffy_regular.sfd tuffy_bold.sfd tuffy_bold_italic.sfd tuffy_italic.sfd Tuffy.ttf Tuffy_Bold.ttf Tuffy_Bold_Italic.ttf Tuffy_Italic.ttf
