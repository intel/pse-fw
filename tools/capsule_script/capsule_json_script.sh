#!/bin/bash
#
# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
# 
device_id_size="unique_device_id_size"
device_id="unique_device_id"
token_id_size="tenant_token_id_size"
token_id="tenant_token_id"
cloud_type_size="cloud_type_size"
cloud_type="cloud_type"
cloud_url_size="cloud_url_size"
cloud_url="cloud_url"
cloud_port="cloud_port"
host_proxy_url_size="proxy_url_size"
host_proxy_url="proxy_url"
host_proxy_port="proxy_port"
client_id="unique_client_id"
client_id_size="unique_client_id_size"
cloud_pem_file="pem_file_path"
cloud_pem_file_size="pem_file_size"
pch_low_mac="PCH_Gbe_Low_Mac"
pch_high_mac="PCH_Gbe_High_Mac"
pse_0_low_mac="PSE_Gbe_0_Low_Mac"
pse_0_high_mac="PSE_Gbe_0_High_Mac"
pse_1_low_mac="PSE_Gbe_1_Low_Mac"
pse_1_high_mac="PSE_Gbe_1_High_Mac"
pse_0_network="PSE_Gbe_0_Network"
pse_0_ipv4="PSE_Gbe_0_Ipv4"
pse_0_ipv6add1="PSE_Gbe_0_Ipv6Add1"
pse_0_ipv6add2="PSE_Gbe_0_Ipv6Add2"
pse_0_ipv6add3="PSE_Gbe_0_Ipv6Add3"
pse_0_ipv6add4="PSE_Gbe_0_Ipv6Add4"
pse_0_subnetmask="PSE_Gbe_0_SubnetMask"
pse_0_gateway="PSE_Gbe_0_Gateway"
pse_0_dnssrvnum="PSE_Gbe_0_DnsSrvNum"
pse_0_dnssrv1add="PSE_Gbe_0_DnsSrv1Add"
pse_0_dnssrv2add="PSE_Gbe_0_DnsSrv2Add"
pse_0_dnssrv3add="PSE_Gbe_0_DnsSrv3Add"
pse_0_dnssrv4add="PSE_Gbe_0_DnsSrv4Add"
pse_1_network="PSE_Gbe_1_Network"
pse_1_ipv4="PSE_Gbe_1_Ipv4"
pse_1_ipv6add1="PSE_Gbe_1_Ipv6Add1"
pse_1_ipv6add2="PSE_Gbe_1_Ipv6Add2"
pse_1_ipv6add3="PSE_Gbe_1_Ipv6Add3"
pse_1_ipv6add4="PSE_Gbe_1_Ipv6Add4"
pse_1_subnetmask="PSE_Gbe_1_SubnetMask"
pse_1_gateway="PSE_Gbe_1_Gateway"
pse_1_dnssrvnum="PSE_Gbe_1_DnsSrvNum"
pse_1_dnssrv1add="PSE_Gbe_1_DnsSrv1Add"
pse_1_dnssrv2add="PSE_Gbe_1_DnsSrv2Add"
pse_1_dnssrv3add="PSE_Gbe_1_DnsSrv3Add"
pse_1_dnssrv4add="PSE_Gbe_1_DnsSrv4Add"
dnssrv_add=(0 0 0 0)

Help()
{
   # Display Help
   echo "This script will generate the capsule binaries for OOB, GBE MAC & TSN capabilities"
   echo "Syntax for OOB capsule: ./capsule_json_script.sh <full path to Cloud.pem file>"
   echo "Syntax for GBE MAC capsule: ./capsule_json_script.sh"
   echo "Syntax for TSN capsule: ./capsule_json_script.sh"
   echo "Syntax for IP capsule: ./capsule_json_script.sh"
   echo "options:"
   echo "h     Print this Help."
   echo
}

################################################################################
# Process the input options.                                                   #
################################################################################
# Get the options
while getopts ":h" option; do
   case $option in
      h) # display Help
         Help
         exit;;
	  \?)
	  # incorrect option
         echo "Error: Invalid option"
		 echo "Use -h for help"
		 exit;;
   esac
done

################################################################################
# Main program                                                                 #
################################################################################

mac_positioning(){
	pos1=${mac_address:0:2}
	pos2=${mac_address:2:2}
	pos3=${mac_address:4:2}
	pos4=${mac_address:6:2}
	pos5=${mac_address:8:2}
	pos6=${mac_address:10:2}
	mac_low=$pos4$pos3$pos2$pos1
	mac_high=0000$pos6$pos5
	echo "$mac_low $mac_high"
}

ip_positioning(){
	IFS='.'
	read -ra pos <<< "$ip"
	printf -v pos1 "%02x" "${pos[0]}"
	printf -v pos2 "%02x" "${pos[1]}"
	printf -v pos3 "%02x" "${pos[2]}"
	printf -v pos4 "%02x" "${pos[3]}"
	pos1=${pos1^^}
	pos2=${pos2^^}
	pos3=${pos3^^}
	pos4=${pos4^^}
	ip=$pos4$pos3$pos2$pos1
	echo "$ip"
}

ipv6_positioning(){
	IFS=':'
	read -ra pos <<< "$ipv6"
	for index in "${!pos[@]}"
	do
		if [[ -z "${pos[$index]}" ]]
		then
			pos[$index]="0000"
		fi
		while [ ${#pos[$index]} -ne 4 ]
		do	
			pos[$index]="0"${pos[$index]}
		done
	done
	pos1=${pos[0]:0:2}
	pos2=${pos[0]:2:2}
	pos3=${pos[1]:0:2}
	pos4=${pos[1]:2:2}
	pos5=${pos[2]:0:2}
	pos6=${pos[2]:2:2}
	pos7=${pos[3]:0:2}
	pos8=${pos[3]:2:2}
	pos9=${pos[4]:0:2}
	pos10=${pos[4]:2:2}
	pos11=${pos[5]:0:2}
	pos12=${pos[5]:2:2}
	add1="0000"$pos2$pos1
	add2="0000"$pos4$pos3
	add3=$pos8$pos7$pos6$pos5
	add4=$pos12$pos11$pos10$pos9
	add1=${add1^^}
	add2=${add2^^}
	add3=${add3^^}
	add4=${add4^^}
	echo "$add1 $add2 $add3 $add4"
}

DHCP_GbE0(){
	sed -i "s/\<$pse_0_network\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_ipv4\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_ipv6add1\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_ipv6add2\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_ipv6add3\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_ipv6add4\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_subnetmask\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_gateway\>/0/g" $IP_file_name
	sed -i "s/\<$pse_0_dnssrvnum\>/0/g" $IP_file_name
	sed -i "s/\<${pse_0_dnssrv1add}\>/0/g" $IP_file_name
	sed -i "s/\<${pse_0_dnssrv2add}\>/0/g" $IP_file_name
	sed -i "s/\<${pse_0_dnssrv3add}\>/0/g" $IP_file_name
	sed -i "s/\<${pse_0_dnssrv4add}\>/0/g" $IP_file_name
	echo ""
	echo "Overview PSE GbE0 IP Configuration"
	echo "**************************"
	echo "*IPv4 :                0 *"
	echo "*IPv4 Subnet Mask :    0 *"
	echo "*IPv4 Gateway Address: 0 *"
	echo "*DNS Server 1 Address: 0 *"
	echo "*DNS Server 2 Address: 0 *"
	echo "*DNS Server 3 Address: 0 *"
	echo "*DNS Server 4 Address: 0 *"
	echo "*IPv6 :                0 *"
	echo "**************************"
}
DHCP_GbE1(){
	sed -i "s/\<$pse_1_network\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_ipv4\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_ipv6add1\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_ipv6add2\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_ipv6add3\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_ipv6add4\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_subnetmask\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_gateway\>/0/g" $IP_file_name
	sed -i "s/\<$pse_1_dnssrvnum\>/0/g" $IP_file_name
	sed -i "s/\<${pse_1_dnssrv1add}\>/0/g" $IP_file_name
	sed -i "s/\<${pse_1_dnssrv2add}\>/0/g" $IP_file_name
	sed -i "s/\<${pse_1_dnssrv3add}\>/0/g" $IP_file_name
	sed -i "s/\<${pse_1_dnssrv4add}\>/0/g" $IP_file_name
	echo ""
	echo "Overview PSE GbE1 IP Configuration"
	echo "**************************"
	echo "*IPv4 :                0 *"
	echo "*IPv4 Subnet Mask :    0 *"
	echo "*IPv4 Gateway Address: 0 *"
	echo "*DNS Server 1 Address: 0 *"
	echo "*DNS Server 2 Address: 0 *"
	echo "*DNS Server 3 Address: 0 *"
	echo "*DNS Server 4 Address: 0 *"
	echo "*IPv6 :                0 *"
	echo "**************************"
}

#downloading certificates required by Generating Capsule step later		
if [[ -f TestCert.pem ]];
then
	echo "TestCert.pem already exists, skip to download"
else
	wget https://raw.githubusercontent.com/tianocore/edk2/master/BaseTools/Source/Python/Pkcs7Sign/TestCert.pem
fi

if [[ -f TestSub.pub.pem ]];
then
	echo "TestSub.pub.pem already exists, skip to download"
else
	wget https://raw.githubusercontent.com/tianocore/edk2/master/BaseTools/Source/Python/Pkcs7Sign/TestSub.pub.pem
fi

if [[ -f TestRoot.pub.pem ]];
then
	echo "TestRoot.pub.pem already exists, skip to download"
else
	wget https://raw.githubusercontent.com/tianocore/edk2/master/BaseTools/Source/Python/Pkcs7Sign/TestRoot.pub.pem
fi

echo "Select Capsule Type"  
select Capsule in "OOB" "TSN" "MAC" "IP"
  do
    case $Capsule in
      "OOB")
        
		if [ -z "$1" ]
		then
			echo -e "\e[1;31m cloud.pem file path not provided \e[0m"
			echo -e "\e[1;31m Exiting from script \e[0m"
			exit 1
		fi

		if [[ ! "$1" =~ ".pem" ]]
		then
			echo -e "\e[1;31m Incomplete path to cloud pem file  \e[0m"
			echo -e "\e[1;31m Exiting from script \e[0m"
			exit 1
		fi
		
		if [[ -f "$1" ]];
		then	
			echo "cloud pem file exists in path provided"
			
			var="$(find $1 -type f -printf '%s\n' 2>&1)"
		else
			echo -e "\e[1;31m cloud.pem file doesnt exist in path provided \e[0m"
			echo -e "\e[1;31m Exiting from script \e[0m"
			exit 1
		fi	
		
		if [[ -f OOBCapsule_template.json ]];
		then	
			read -p "Enter Capsule file name(without extension): " OOB_file_name
			OOB_Capsule_name=$OOB_file_name.bin
			OOB_file_name=$OOB_file_name.json			
			cp OOBCapsule_template.json $OOB_file_name
		else
			echo -e "\e[1;31m OOBCapsule_template.json does not exist, please copy the template file to script directory \e[0m"
			exit 1
		fi
		
		echo "Select Cloud Service Provider"
		select Cloud in "Telit" "Azure" "ThingsBoard"
		do
			case $Cloud in
				"Telit")
					sed -i "s/\<$cloud_type_size\>/5/g" $OOB_file_name
					sed -i "s/\<$cloud_type\>/Telit/g" $OOB_file_name
					
					read -p "Enter device id: " new_device_id
					sed -i "s/\<$device_id_size\>/${#new_device_id}/g" $OOB_file_name
					sed -i "s/\<$device_id\>/$new_device_id/g" $OOB_file_name
		
					read -p "Enter token id: " new_token_id
					sed -i "s/\<$token_id_size\>/${#new_token_id}/g" $OOB_file_name
					sed -i "s/\<$token_id\>/$new_token_id/g" $OOB_file_name
					
					new_client_id="elkhart_oob"
					sed -i "s/\<$client_id_size\>/${#new_client_id}/g" $OOB_file_name
					sed -i "s/\<$client_id\>/$new_client_id/g" $OOB_file_name
					
					echo "Enter Telit Cloud Type "
					select cloud_type in "Production" "Development"
						do
							case $cloud_type in
								"Production")
										new_cloud_url=api-us.devicewise.com
										sed -i "s/$cloud_url_size/${#new_cloud_url}/g" $OOB_file_name
										sed -i "s/$cloud_url/$new_cloud_url/g" $OOB_file_name
										break;;
								"Development")
										new_cloud_url=api-dev.devicewise.com
										sed -i "s/$cloud_url_size/${#new_cloud_url}/g" $OOB_file_name
										sed -i "s/$cloud_url/$new_cloud_url/g" $OOB_file_name
										break;;
									*) 
										echo -e "\e[1;31m Invalid option! \e[0m"
										rm -rf $OOB_file_name
										echo -e "\e[1;31m Exiting from script \e[0m"
										exit 1;;
							esac
						done
					break;;
				"Azure")
					sed -i "s/\<$cloud_type_size\>/5/g" $OOB_file_name
					sed -i "s/\<$cloud_type\>/Azure/g" $OOB_file_name

					if [[ -f azure_credentials.py ]];
					then	
						read -p "Enter Scope id: " new_scope_id
						read -p "Enter device id: " new_client_id
						read -p "Enter Primary Key: " new_prim_key
					else
						echo -e "\e[1;31m azure_credentials.py does not exist, please copy the template file to script directory \e[0m"
						rm -rf $OOB_file_name
						echo -e "\e[1;31m Exiting from script \e[0m"
						exit 1
					fi
					
					azure_cred="$(python3 azure_credentials.py $new_scope_id $new_client_id $new_prim_key 2>&1)"
					if [ "$?" -eq "0" ]; then
						if [[ ! $azure_cred =~ "error" ]]
						then
							readarray -t y <<<"$azure_cred"
							new_device_id="$(echo ${y[0]})"
							new_token_id="$(echo ${y[1]})"
						else 
							echo -e "\e[1;31m Invalid azure cloud params \e[0m"
							rm -rf $OOB_file_name
							echo -e "\e[1;31m Exiting from script \e[0m"	
							exit 1
						fi
					else
						echo -e "\e[1;31m Invalid azure cloud params \e[0m"
						rm -rf $OOB_file_name
						echo -e "\e[1;31m Exiting from script \e[0m"
						exit 1
					fi 
										
					sed -i "s/\<$token_id_size\>/${#new_token_id}/g" $OOB_file_name
					sed -i "s#$token_id#${new_token_id//&/\\&}#g" $OOB_file_name

					sed -i "s/\<$device_id_size\>/${#new_device_id}/g" $OOB_file_name
					sed -i "s#$device_id#${new_device_id//&/\\&}#g" $OOB_file_name

					sed -i "s/\<$client_id_size\>/${#new_client_id}/g" $OOB_file_name
					sed -i "s/\<$client_id\>/$new_client_id/g" $OOB_file_name

					new_cloud_url="$(cut -d'/' -f1 <<<"$new_device_id")"
					sed -i "s/$cloud_url_size/${#new_cloud_url}/g" $OOB_file_name
					sed -i "s/$cloud_url/$new_cloud_url/g" $OOB_file_name
					
					break;;
				"ThingsBoard")
					sed -i "s/\<$cloud_type_size\>/11/g" $OOB_file_name
					sed -i "s/\<$cloud_type\>/ThingsBoard/g" $OOB_file_name
					
					read -p "Enter device id (Access Token from ThingsBoard portal): " new_device_id
					sed -i "s/\<$device_id_size\>/${#new_device_id}/g" $OOB_file_name
					sed -i "s/\<$device_id\>/$new_device_id/g" $OOB_file_name
		
					new_token_id="none"
					sed -i "s/\<$token_id_size\>/${#new_token_id}/g" $OOB_file_name
					sed -i "s/\<$token_id\>/$new_token_id/g" $OOB_file_name
					
					new_client_id="elkhart_oob"
					sed -i "s/\<$client_id_size\>/${#new_client_id}/g" $OOB_file_name
					sed -i "s/\<$client_id\>/$new_client_id/g" $OOB_file_name
					
					read -p "Enter Cloud URL: " new_cloud_url
					sed -i "s/$cloud_url_size/${#new_cloud_url}/g" $OOB_file_name
					sed -i "s/$cloud_url/$new_cloud_url/g" $OOB_file_name
					
					break;;
				*) 
					echo -e "\e[1;31m Invalid option! \e[0m"
					rm -rf $OOB_file_name
					echo -e "\e[1;31m Exiting from script \e[0m"
					exit 1;;
			esac
		done
        
		read -p "Enter Proxy (SOCKS5) IP: " new_proxy_url
		sed -i "s/$host_proxy_url_size/${#new_proxy_url}/g" $OOB_file_name
		sed -i "s/$host_proxy_url/$new_proxy_url/g" $OOB_file_name
		
		sed -i "s/\<$cloud_port\>/8883/g" $OOB_file_name
		sed -i "s/\<$host_proxy_port\>/1080/g" $OOB_file_name
		
		sed -i "s#$cloud_pem_file#$1#g" $OOB_file_name
		sed -i "s/$cloud_pem_file_size/$var/g" $OOB_file_name
		
		echo -e "\e[1;32m $OOB_file_name is generated successfully \e[0m"
		
		#Generating Capsule
		if [[ -f subregion_capsule.py ]];
		then
			python3 subregion_capsule.py -o $OOB_Capsule_name -s TestCert.pem -p TestSub.pub.pem -t TestRoot.pub.pem $OOB_file_name
		else
			echo -e "\e[1;31m subregion_capsule.py does not exist \e[0m"
			echo -e "\e[1;31m Exiting from script \e[0m"
			exit 1
		fi
		
		echo -e "\e[1;32m $OOB_Capsule_name is generated successfully \e[0m"		
		
		break;;
      "TSN")
	  	echo -e "\e[1;31m TSN capsule not enabled yet \e[0m"
		echo -e "\e[1;31m Exiting from script \e[0m"
        break;;
      "MAC")
		echo "You will be prompted few times to key in all 3 ethernet ports MAC values"
                echo "Do not enter blank values, but a proper MAC values"
 
		if [[ -f mac_template.json ]];
		then	
			read -p "Enter Capsule file name(without extension): " MAC_file_name
			
			MAC_Capsule_name=$MAC_file_name.bin
			MAC_file_name=$MAC_file_name.json	
			cp mac_template.json $MAC_file_name
		else
			echo -e "\e[1;31m mac_template.json does not exist, please copy the template file to script directory \e[0m"
			exit 1
		fi

		x=0
		while [ $x -le 0 ] 
		do
			echo "Fill in the hex value of the mac label without symbol"
			read -p "Enter Mac Address for PCH GbE: " mac_pch
	        	mac_address=${mac_pch^^}
			if [ `echo $mac_address | egrep "^([0-9A-F]{2}){5}[0-9A-F]{2}$"` ]
			then
				read mac_low mac_high < <(mac_positioning $mac_address)
				sed -i "s/\<$pch_low_mac\>/$mac_low/g" $MAC_file_name
				sed -i "s/\<$pch_high_mac\>/$mac_high/g" $MAC_file_name
				x=1
			else
				echo -e "\e[1;31m Invalid Mac Address \e[0m"
				echo -e "\e[1;31m Please Try again \e[0m"
   			fi
		done

		x=0
		while [ $x -le 0 ]
		do
			echo "Fill in the hex value of the mac label without symbol"
        		read -p "Enter Mac Address for PSE GbE0: " mac_pse_gbe0
        		mac_address=${mac_pse_gbe0^^}
       			if [ `echo $mac_address | egrep "^([0-9A-F]{2}){5}[0-9A-F]{2}$"` ]
        		then
				read mac_low mac_high < <(mac_positioning $mac_address)
				sed -i "s/\<$pse_0_low_mac\>/$mac_low/g" $MAC_file_name
				sed -i "s/\<$pse_0_high_mac\>/$mac_high/g" $MAC_file_name
				x=1
			else
				echo -e "\e[1;31m Invalid Mac Address \e[0m"
				echo -e "\e[1;31m Please Try again \e[0m"
				echo ""
        	fi
		done
		
		x=0
		while  [ $x -le 0 ]
		do
			echo "Fill in the hex value of the mac label without symbol"
			read -p "Enter Mac Address for PSE GbE1: " mac_pse_gbe1
			mac_address=${mac_pse_gbe1^^}
			if [ `echo $mac_address | egrep "^([0-9A-F]{2}){5}[0-9A-F]{2}$"` ]
			then
				read mac_low mac_high < <(mac_positioning $mac_address)
				sed -i "s/\<$pse_1_low_mac\>/$mac_low/g" $MAC_file_name
				sed -i "s/\<$pse_1_high_mac\>/$mac_high/g" $MAC_file_name
				x=1
			else
				echo -e "\e[1;31m Invalid Mac Address \e[0m"
				echo -e "\e[1;31m Please Try again \e[0m"
				echo ""
       	 	fi
		done

       	echo "Overview MAC address Configuration"
       	echo "*******************************"
		echo "*MAC address of PCH GbE : $mac_pch       *"
		echo "*MAC address of PSE GbE0: $mac_pse_gbe0*"
		echo "*MAC address of PSE GbE1: $mac_pse_gbe1*"
		echo "*******************************"
		echo -e "\e[1;32m $MAC_file_name is generated successfully \e[0m"
		
		#Generating Capsule
		if [[ -f subregion_capsule.py ]];
		then
			python3 subregion_capsule.py -o $MAC_Capsule_name -s TestCert.pem -p TestSub.pub.pem -t TestRoot.pub.pem $MAC_file_name
		else
			echo -e "\e[1;31m subregion_capsule.py does not exist \e[0m"
			echo -e "\e[1;31m Exiting from script \e[0m"
			exit 1
		fi
		
		echo -e "\e[1;32m $MAC_Capsule_name is generated successfully \e[0m"		

        break;;

	    "IP")
	 	if [[ -f ip_template.json ]];
		then	
			read -p "Enter Capsule file name(without extension): " IP_file_name
			
			IP_Capsule_bin=$IP_file_name.bin
			IP_file_name=$IP_file_name.json	
			cp ip_template.json $IP_file_name
		else
			echo -e "\e[1;31m ip_template.json does not exist, please copy the template file to script directory \e[0m"
			exit 1
		fi
	 	echo "Configure IP for PSE GbE0"
		echo "Select IP Assignment Type"  
		select Network in "Static" "DHCP" "Skip to Configure"
		do
		case $Network in
			"Static")
			sed -i "s/\<$pse_0_network\>/1/g" $IP_file_name
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv4 (e.g. 192.168.1.10)"
				read -p "Enter Ipv4 Address: "  ipv4_gbe0
				test='([1-9]?[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])'
				if [[ $ipv4_gbe0 =~ ^$test\.$test\.$test\.$test$ ]]
				then
					ip=$ipv4_gbe0
					read ip < <(ip_positioning $ip)
					sed -i "s/\<$pse_0_ipv4\>/\"$ip\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo -e "\e[1;31m Invalid IPv4 \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv4 Subnet Mask (e.g. 255.255.254.0)"
				read -p "Enter subnet Address: "  subnet
				if [[ $subnet =~ ^$test\.$test\.$test\.$test$ ]]
				then
					ip=$subnet
					read ip < <(ip_positioning $ip)
					sed -i "s/\<$pse_0_subnetmask\>/\"$ip\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo -e "\e[1;31m Invalid IPv4 Subnet Mask \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv4 Gateway Address (e.g. 192.168.1.1)"
				read -p "Enter Gateway Address: "  gateway
				if [[ $gateway =~ ^$test\.$test\.$test\.$test$ ]]
				then
					ip=$gateway
					read ip < <(ip_positioning $ip)
					sed -i "s/\<$pse_0_gateway\>/\"$ip\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo -e "\e[1;31m Invalid IPv4 Gateway Address \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Number of DNS server to configure (e.g. 0-4)"
				read -p "Enter Number of DNS Server to configure: "  num
				if (($num > 0 & $num <= 4))
				then
					dnssrv_add=(0 0 0 0)
					while [ $num -ne 0 ]
					do
						sed -i "s/\<$pse_0_dnssrvnum\>/$num/g" $IP_file_name
						echo ""
						echo "Fill in DNS server ${num} address (e.g. 8.8.4.4)"
						read -p "Enter DNS Server ${num} address: "  dns
						if [[ $dns =~ ^$test\.$test\.$test\.$test$ ]]
						then
							ip=$dns
							read ip < <(ip_positioning $ip)
							dnssrv_add[$num-1]=\"$ip\"
							(( num-- ))
							x=1
						else
							echo -e "\e[1;31m Invalid DNS Address \e[0m"
							echo -e "\e[1;31m Please Try again \e[0m"                              
						fi
					done
				elif [[ $num == 0 ]]
				then
					sed -i "s/\<$pse_0_dnssrvnum\>/$num/g" $IP_file_name
					x=1
				else
					echo -e "\e[1;31m Invalid Number \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
				fi
			done
			sed -i "s/\<${pse_0_dnssrv1add}\>/${dnssrv_add[0]}/g" $IP_file_name
			sed -i "s/\<${pse_0_dnssrv2add}\>/${dnssrv_add[1]}/g" $IP_file_name
			sed -i "s/\<${pse_0_dnssrv3add}\>/${dnssrv_add[2]}/g" $IP_file_name
			sed -i "s/\<${pse_0_dnssrv4add}\>/${dnssrv_add[3]}/g" $IP_file_name
			echo " "
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv6 (e.g. fe80::c470:09ff:feb1:54a9)"
				read -p "Enter Ipv6 Address: "  ipv6_gbe0
				if [ `echo $ipv6_gbe0 | grep "^\([0-9a-fA-F]\{0,4\}:\)\{1,7\}[0-9a-fA-F]\{0,4\}$"` ]
				then
					ipv6=$ipv6_gbe0
					read add1 add2 add3 add4 < <(ipv6_positioning $ipv6)
					sed -i "s/\<$pse_0_ipv6add1\>/\"$add1\"/g" $IP_file_name
					sed -i "s/\<$pse_0_ipv6add2\>/\"$add2\"/g" $IP_file_name
					sed -i "s/\<$pse_0_ipv6add3\>/\"$add3\"/g" $IP_file_name
					sed -i "s/\<$pse_0_ipv6add4\>/\"$add4\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo -e "\e[1;31m Invalid IPv6 \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			echo ""
			echo ""
			echo "Overview PSE GbE0 IP Configuration"
			echo "***********************************"
			echo "*IPv4 :              $ipv4_gbe0*"
			echo "*Subnet Mask :       $subnet*"
			echo "*IPv4 Gateway Address: $gateway*"
			echo "*DNS Server 1 Address: ${dnssrv_add[0]}          *"
			echo "*DNS Server 2 Address: ${dnssrv_add[1]}          *"
			echo "*DNS Server 3 Address: ${dnssrv_add[2]}          *"
			echo "*DNS Server 4 Address: ${dnssrv_add[3]}          *"
			echo "*IPv6 : $ipv6_gbe0 *"
			echo "***********************************"
			echo ""
			break;;

			"DHCP")
				DHCP_GbE0
			break;;

			"Skip to Configure")
				echo ""
				DHCP_GbE0
			break;;
		esac
 	done
	echo "Configure IP for PSE GbE1"
	echo "Select IP Assignment Type"  
	select Network in "Static" "DHCP" "Skip to Configure"
	do
	case $Network in
		"Static")
			sed -i "s/\<$pse_1_network\>/1/g" $IP_file_name
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv4 (e.g. 192.168.1.10)"
				read -p "Enter Ipv4 Address: "  ipv4_gbe1
				test='([1-9]?[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])'
				if [ $ipv4_gbe1 == $ipv4_gbe0 ]
				then
					echo -e "\e[1;31m Duplicate IPv4 detected\e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				elif [[ $ipv4_gbe1 =~ ^$test\.$test\.$test\.$test$ ]]
				then
					ip=$ipv4_gbe1
					read ip < <(ip_positioning $ip)
					sed -i "s/\<$pse_1_ipv4\>/\"$ip\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo ""
					echo -e "\e[1;31m Invalid IPv4 \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv4 Subnet Mask (e.g. 255.255.254.0)"
				read -p "Enter subnet Address: "  subnet
				if [[ $subnet =~ ^$test\.$test\.$test\.$test$ ]]
				then
					ip=$subnet
					read ip < <(ip_positioning $ip)
					sed -i "s/\<$pse_1_subnetmask\>/\"$ip\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo -e "\e[1;31m Invalid IPv4 Subnet Mask \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv4 Gateway Address (e.g. 192.168.1.1)"
				read -p "Enter Gateway Address: "  gateway
				if [[ $gateway =~ ^$test\.$test\.$test\.$test$ ]]
				then
					ip=$gateway
					read ip < <(ip_positioning $ip)
					sed -i "s/\<$pse_1_gateway\>/\"$ip\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo -e "\e[1;31m Invalid IPv4 Gateway Address \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Number of DNS server to configure (e.g. 0-4)"
				read -p "Enter Number of DNS Server to configure: "  num
				if (($num > 0 & $num <= 4))
				then
					dnssrv_add=(0 0 0 0)
					while [ $num -ne 0 ]
					do
						sed -i "s/\<$pse_1_dnssrvnum\>/$num/g" $IP_file_name
						echo ""
						echo "Fill in DNS server ${num} address (e.g. 8.8.4.4)"
						read -p "Enter DNS Server ${num} address: "  dns
						if [[ $dns =~ ^$test\.$test\.$test\.$test$ ]]
						then
							ip=$dns
							read ip < <(ip_positioning $ip)
							dnssrv_add[$num-1]=\"$ip\"
							(( num-- ))
							x=1
						else
							echo -e "\e[1;31m Invalid DNS Address \e[0m"
							echo -e "\e[1;31m Please Try again \e[0m"                              
						fi
					done
				elif [[ $num == 0 ]]
				then 
					sed -i "s/\<$pse_1_dnssrvnum\>/$num/g" $IP_file_name
					x=1
				else
					echo -e "\e[1;31m Invalid Number \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
				fi
			done
			sed -i "s/\<${pse_1_dnssrv1add}\>/${dnssrv_add[0]}/g" $IP_file_name
			sed -i "s/\<${pse_1_dnssrv2add}\>/${dnssrv_add[1]}/g" $IP_file_name
			sed -i "s/\<${pse_1_dnssrv3add}\>/${dnssrv_add[2]}/g" $IP_file_name
			sed -i "s/\<${pse_1_dnssrv4add}\>/${dnssrv_add[3]}/g" $IP_file_name
			echo " "
			x=0
			while  [ $x -le 0 ]
			do
				echo "Fill in Static IPv6 (e.g. fe80::c470:09ff:feb1:54a9)"
				read -p "Enter Ipv6 Address: "  ipv6_gbe1
				if [ $ipv6_gbe1 == $ipv6_gbe0 ]
				then
					echo -e "\e[1;31m Duplicate IPv6 detected\e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				elif [ `echo $ipv6_gbe1 | grep "^\([0-9a-fA-F]\{0,4\}:\)\{1,7\}[0-9a-fA-F]\{0,4\}$"` ]
				then
					ipv6=$ipv6_gbe1
					read add1 add2 add3 add4 < <(ipv6_positioning $ipv6)
					sed -i "s/\<$pse_1_ipv6add1\>/\"$add1\"/g" $IP_file_name
					sed -i "s/\<$pse_1_ipv6add2\>/\"$add2\"/g" $IP_file_name
					sed -i "s/\<$pse_1_ipv6add3\>/\"$add3\"/g" $IP_file_name
					sed -i "s/\<$pse_1_ipv6add4\>/\"$add4\"/g" $IP_file_name
					x=1
					echo ""
				else
					echo -e "\e[1;31m Invalid IPv6 \e[0m"
					echo -e "\e[1;31m Please Try again \e[0m"
					echo ""
				fi
			done
			echo ""
			echo ""
			echo "Overview PSE GbE1 Configuration"
			echo "***********************************"
			echo "*IPv4 :              $ipv4_gbe1*"
			echo "*Subnet Mask :       $subnet*"
			echo "*IPv4 Gateway Address: $gateway*"
			echo "*DNS Server 1 Address: ${dnssrv_add[0]}          *"
			echo "*DNS Server 2 Address: ${dnssrv_add[1]}          *"
			echo "*DNS Server 3 Address: ${dnssrv_add[2]}          *"
			echo "*DNS Server 4 Address: ${dnssrv_add[3]}          *"
			echo "*IPv6 : $ipv6_gbe1 *"
			echo "***********************************"
			break;;

		"DHCP")
			DHCP_GbE1
			break;;

		"Skip to Configure")
			DHCP_GbE1
			break;;
		esac
 	done
	echo -e "\e[1;32m $IP_file_name is generated successfully \e[0m"
	if [[ -f subregion_capsule.py ]];
	then
		python3 subregion_capsule.py -o $IP_Capsule_bin -s TestCert.pem -p TestSub.pub.pem -t TestRoot.pub.pem $IP_file_name
	else
		echo -e "\e[1;31m subregion_capsule.py does not exist \e[0m"
		echo -e "\e[1;31m Exiting from script \e[0m"
		exit 1
	fi
	echo -e "\e[1;32m $IP_Capsule_bin is generated successfully \e[0m"
	break;; 
    esac
  done
exit 0


