work_dir="__test__"
tar_file="_run_.tar.gz"
script_file="h1.sh"
script_args="sub.tar.gz sol.c stackoverflow.com"
report_file="report.txt"

rm -rf $work_dir
mkdir -p $work_dir
tar -xzf $tar_file -C $work_dir
cp $script_file ${work_dir}/${script_file}

cd $work_dir
touch "DO_NOT_DELETE" # Some "other stuff in directory"

echo "files before script"
ls .
# echo "####################\n"
echo

echo "running $script_file"
./$script_file $script_args
echo

echo "files after script"
ls .
# echo "####################"
