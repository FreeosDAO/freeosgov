echo -e "freeosgov2 POINT balance:"
cleos -u https://protontestnet.greymass.com get table freeosgov2 freeosgov2 mintfeefree
echo -e "\nPOINT supply:"
cleos -u https://protontestnet.greymass.com get table freeosgov2 POINT stat
echo -e "\nfreeosgov2 FREEBI balance:"
cleos -u https://protontestnet.greymass.com get table freebi freeosgov2 accounts
echo -e "\nFREEBI supply:"
cleos -u https://protontestnet.greymass.com get table freebi FREEBI stat
echo -e "\nfreeosgov2 FREEOS balance:"
cleos -u https://protontestnet.greymass.com get table freeostokens freeosgov2 accounts
echo -e "\nFREEOS supply:"
cleos -u https://protontestnet.greymass.com get table freeostokens FREEOS stat
