#!/bin/bash

server_name=$(hostname)

function optionsdiv(){
eosio-cpp -abigen dividenda.cpp -o=depl_optionsdiv/dividenda/dividenda.wasm -I=incl_optionsdiv
}

function optionsdiva(){
eosio-cpp -abigen dividenda.cpp -o=depl_optionsdiva/dividenda/dividenda.wasm -I=incl_optionsdiva
}

function freeosdiv(){
eosio-cpp -abigen dividenda.cpp -o=depl_freeosdiv/dividenda/dividenda.wasm -I=incl_freeosdiv
}

function all_codes(){
eosio-cpp -abigen dividenda.cpp -o=depl_optionsdiv/dividenda/dividenda.wasm -I=incl_optionsdiv
eosio-cpp -abigen dividenda.cpp -o=depl_optionsdiva/dividenda/dividenda.wasm -I=incl_optionsdiva
eosio-cpp -abigen dividenda.cpp -o=depl_freeosdiv/dividenda/dividenda.wasm -I=incl_freeosdiv
}

function deploy_odiv(){
command proton1 set contract optionsdiv /home/andy/optionsdiv/Dividend/depl_optionsdiv/dividenda -p optionsdiv
}

function deploy_odiva(){
proton1 set contract optionsdiva /home/andy/optionsdiv/Dividend/depl_optionsdiva/dividenda -p optionsdiva
}

function deploy_fred(){
proton1 set contract freeosdiv /home/andy/optionsdiv/Dividend/depl_freeosdiv/dividenda -p freeosdiv
}

function deploy_all(){
proton1 set contract optionsdiv /home/andy/optionsdiv/Dividend/depl_optionsdiv/dividenda -p optionsdiv
proton1 set contract optionsdiva /home/andy/optionsdiv/Dividend/depl_optionsdiva/dividenda -p optionsdiva
proton1 set contract freeosdiv /home/andyb/dividenda/Dividend/depl_freeosdiv/dividenda -p freeosdiv
proton1 set contract optionsdiv1 /home/andy/optionsdiv/Dividend/depl_optionsdiv1/dividenda -p optionsdiv1
}

function optionsdiv1(){
eosio-cpp -abigen dividenda.cpp -o=depl_optionsdiv1/dividenda/dividenda.wasm -I=incl_optionsdiv1
}

##
# Color  Variables
##
green='\e[32m'
blue='\e[34m'
clear='\e[0m'

##
# Color Functions
##

ColorGreen(){
	echo -ne $green$1$clear
}
ColorBlue(){
	echo -ne $blue$1$clear
}

menu(){
echo -ne "
Compile Menu
$(ColorGreen '1)') compile optionsdiv
$(ColorGreen '2)') compile optionsdiva
$(ColorGreen '3)') compile freeosdiv
$(ColorGreen '4)') compile all
$(ColorBlue '5)') deploy optionsdiv
$(ColorBlue '6)') deploy optionsdiva
$(ColorBlue '7)') deploy freeosdiv
$(ColorBlue '8)') deploy all
$(ColorBlue '9)') compile alpha (optionsdiv1)
$(ColorGreen '0)') Exit
$(ColorBlue 'Choose an option:') "
        read a
        case $a in
	        1) optionsdiv ; menu ;;
	        2) optionsdiva ; menu ;;
	        3) freeosdiv ; menu ;;
	        4) all_codes ; menu ;;
          5) deploy_odiv; menu ;;
          6) deploy_odiva; menu ;;
          7) deploy_fred; menu ;;
          8) deploy_all; menu ;;
          9) optionsdiv1 ; menu ;;
		0) exit 0 ;;
		*) echo -e $red"Wrong option."$clear; WrongCommand;;
        esac
}

# Call the menu function
menu
