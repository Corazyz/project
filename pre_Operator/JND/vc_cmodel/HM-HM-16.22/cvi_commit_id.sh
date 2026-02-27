
echo "Script executed from: ${1}"
message="$( git rev-parse HEAD )"
echo -n "const string commit_id = \"" > ${1}/source/Lib/TLibCommon/cvi_commid_id.h
echo -n "$message\";" >> ${1}/source/Lib/TLibCommon/cvi_commid_id.h
