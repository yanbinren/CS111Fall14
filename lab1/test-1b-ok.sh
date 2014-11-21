#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

# Copyright 2012-2014 Paul Eggert.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
#simple
exec echo start testing

#sub shell
( echo subshell_succeeded )

#sequence
touch cc ; echo sequence succeed > cc

#if
if cat cc
then echo if_succeeded
else echo if_failed
fi

echo while_succeeded > ee

#while
while cat ee
do rm ee
done

#pipe
echo pipe_succeeded | sort

#until
until
cat dd
do echo until_succeeded > dd
done

EOF

cat >test.exp <<'EOF'
start testing
subshell_succeeded
sequence succeed
if_succeeded
while_succeeded
pipe_succeeded
until_succeeded
EOF

../profsh test.sh >test.out 2>test.err

diff -u test.exp test.out

cat test.out
#test ! -s test.err || {
#  cat test.err
#  exit 1
#}

) || exit

rm -fr "$tmp"

