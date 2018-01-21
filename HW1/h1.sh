
submissions=$1
solustion=$2
keyword=$3

if [[ $# -ne 3 ]]; then
	echo "Usage: $0 submissions.tar.gz solution.c keyword"
	exit 1
fi

mkdir -p _temp_
tar -xf $1 -C _temp_
