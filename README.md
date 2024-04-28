***Cazacu Alexandru Dan***
***312 CA***

# Distributed Database

## Elemente ale bazei de date

### Server
Pentru a retine informatii despre documentele transmise prin requesturi, se folosesc mai 
multe servere adaugate prin comanda ***"ADD_SERVER"*** care contin o baza de data 
locala, o coada de task-uri ce asteapta a fi executate, dar si un nivel de caching pentru a 
accesa mai repede documentele.

### Load Balancer
Load Balancerul reprezinta o componenta esentiala a distributiei informatiilor in baza de 
data. Acesta stocheaza toate informatiile necesare despre serverele care cuprind aceasta 
baza de date, dar si informatii despre functiile de hashing ale serverelor si ale 
documentelor si despre numarul de replici.

## Requesturi si comenzi

### EDIT
Comanda ***"EDIT <document_name> <document_content>"*** adauga/modifica 
documentul cu numele respectiv din baza de date. Documentul cu acest nume, dupa 
acest request, va avea drept continut noile informatii transmise prin request. Operatia de 
editare la nivelul serverului se va desfasura in 2 etape: cautarea documentului in cache-
ul serverului, dar si in baza lui de date (intrucat continutul va fi modificat pe ambele 
nivele). Exista, deci, 3 moduri in care un document este editat: documentul exista si in 
cache si in baza de date, documentul exista doar in baza de date, documentul nu exista. 
Toate aceste cazuri sunt tratate relativ in acelasi mod, noul document impreuna cu 
continutul lui vor fi stocate in cache si in baza de date la finalul taskului, fie daca este 
creat sau modificat. Raspunsurile trimise de server sunt specifice fiecarui caz in care este 
executat cazul.

### GET
Comanda ***"GET <document_name>"*** trimite ca raspuns continutul documentului cu 
respectivul nume. Acesta este cautat prima data la nivelul cache-ului, iar in cazul gasirii, 
se transmite direct continutul. In cazul in care documentul nu este prezent in cache, se 
face o cautare a acestuia in baza de date locala. Daca acesta este gasit, se va da drept 
raspuns continutul acestuia, dar daca acesta nu exista nici in baza de date, raspunsul va 
contine NULL.

### ADD SERVER
Comanda ***"ADD SERVER <server_id>"*** adauga in hash ringul curent un anumit 
server. Executia acestei comenzi consta in calcularea hash-urilor asociate fiecare replici 
ale serverului si gasirea primului server din hash ring care se afla la stanga fiecarei replici. 
Noului server i se vor transmite datele din serverul aflat dupa el pe hash ring, astfel incat 
documentele sa se afle la dreapta serverului adaugat. Cautarea primului server aflat in 
stanga unui anumit hash se face in 2 moduri: fie hash-ul relativ la hash ring nu este 
"ultimul", deci se va cauta primul server cu hashul minim, dar mai mare decat cel cautat, 
in caz contrar, hashul este "ultimul" pe hash ring si se cauta serverul cu hash-ul minim. 
Inaintea oricarui transfer de document se va face executia cozii de task-uri pe serverul 
de pe care se transmite.

### REMOVE SERVER
Comanda ***"REMOVE <server_id>"*** scoate din hash ring serverul cu un anumit ID. 
Pentru a realiza eliminarea corecta a acestuia, se cauta, exact ca la adaugarea unui 
server, serverul care se afla la stanga celui scos in hash ring. In aceasta situatie, vor fi 
transferate intre cele 2 servere toate documentele care se afla pe serverul eliminat. 
Inaintea acestor schimburi de documente, de asemenea, se va face executia cozii de 
task-uri pe serverul eliminat.

### Log-uri
Pentru oricare dintre operatiile care folosesc cautarea sau adaugarea in cache, se vor 
transmite prin intermediul raspunsurilor, log-uri ce privesc informatiile aflate in cache. 
Astfel, la gasirea unui document la nivelul cache, se va transmite un log de tipul **"Cache 
HIT for <document_name>"**. In caz contrar, log-ul primit ca raspuns este **"Cache MISS 
for <document_name>"**. Avand in vedere si eliminarea perechilor (cheie, valoare) in 
cazul in care cacheul isi atinge limita, se va transmite mesajul **"Cache MISS for 
<document_name> - cache entry for <evicted_document_name> has been evicted"**.

