#!/bin/bash

chmod +x pseint/pseint pseint/psexport
cp -f pseint/pseint /usr/bin/pseint
cp -f pseint/psexport /usr/bin/psexport

cp -f ./anti_cheating/anti_cheating.sh /usr/bin/anti_cheating.sh
chmod +x /usr/bin/anti_cheating.sh

service judged stop
cd $PWD/judge
make
chmod +x judged
cp -f  judged /usr/bin

cd ../judge_client
make clean
make
chmod +x judge_client
cp -f judge_client /usr/bin/

cp judged /etc/init.d/judged
chmod +x  /etc/init.d/judged

ln -sf /etc/init.d/judged /etc/rc3.d/S93judged
ln -sf /etc/init.d/judged /etc/rc2.d/S93judged

systemctl daemon-reload
service judged start
