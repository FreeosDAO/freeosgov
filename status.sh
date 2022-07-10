echo -e "mint fee free allowance:"
cleos -u https://protontestnet.greymass.com get table freeosgov2 $1 mintfeefree
echo -e "\nXPR balance:"
cleos -u https://protontestnet.greymass.com get table eosio.token $1 accounts
echo -e "\nFREEBI balance:"
cleos -u https://protontestnet.greymass.com get table freebi $1 accounts
echo -e "\nCREDITS:"
cleos -u https://protontestnet.greymass.com get table freeosgov2 vivvestin credits
echo -e "\nPOINT balance:"
cleos -u https://protontestnet.greymass.com get table freeosgov2 vivvestin accounts
echo -e "\nFREEOS balance:"
cleos -u https://protontestnet.greymass.com get table freeostokens $1 accounts
