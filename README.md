Per compilare usare:
> source ~/rm-sdk/environment-setup-*-remarkable-linux
> mkdir -p build && cd build
> cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/mirtillo
> cmake --build . -j

Per testare 
########################################################
NOTA- Non più perseguibile dopo l'aggironamento a 3.23.*
#########################################################
> ssh root@10.11.99.1 'mkdir -p /opt/mirtillo/bin'. 
> scp build/mirtillo root@10.11.99.1:/opt/mirtillo/bin/
> ssh root@10.11.99.1 '/opt/mirtillo/bin/mirtillo'

Dopo gli aggionramenti di reMarkable 3.23.*
> scp post-update-mirtillo-setup.sh root@10.11.99.1:/home/root
> ssh root@10.11.99.1 'sh ~/post-update-mirtillo-setup.sh'
> # poi chiudi e riapri la sessione SSH (oppure):
ssh root@10.11.99.1 '. ~/.profile && echo $PATH && echo $LD_LIBRARY_PATH'

Poi copiare il binario:
> scp build/mirtillo root@10.11.99.1:~/.local/bin/

