# Alexa : émulation Philips Hue et Belkin Wemo

Cette intégration fonctionne sur le réseau local, via SSDP et HTTP sur le port 80.
Elle ne nécessite pas de skill Alexa ni de compte cloud OpenBeken.
La découverte dépend du modèle et du logiciel de l'Echo ; elle doit être vérifiée
sur le matériel réel. Le téléphone seul ne remplace pas un Echo pour cette découverte locale.

## Philips Hue : un appareil par relais

Dans la console OpenBeken, lancer :

```text
backlog startDriver SSDP; startDriver Hue
```

Pour conserver cette activation après un redémarrage, ajouter les deux commandes
au script `autoexec.bat` dans LittleFS, en conservant son contenu existant :

```text
startDriver SSDP
startDriver Hue
```

Configurer d'abord les GPIO du module : chaque relais doit avoir son canal distinct.
Les canaux de type `Toggle` sont également exposés. Les entrées, capteurs et sorties
PWM ne sont pas exposés par cette implémentation Hue, destinée aux relais On/Off.
Les numéros de canal OpenBeken sont conservés, y compris 0 et 63.

Le nom Alexa vient du libellé de canal (`SetChannelLabel`), ou du nom du module
suivi du numéro de canal. Les identifiants de lumière sont stables, dérivés de la MAC
et du canal. Ils diffèrent de ceux de l'ancien pilote Switch-Gang : supprimer les
anciennes lumières dans Alexa si un appareil utilise déjà ce pilote.

Lancer ensuite la recherche d'appareils Alexa, avec l'Echo et le module sur un réseau
autorisant le multicast SSDP (UDP 239.255.255.250:1900) et HTTP. Le réseau invité et
l'isolation des clients Wi-Fi peuvent empêcher cette découverte.

### Vérification HTTP

Remplacer `IP_DU_MODULE` par l'adresse du module :

```sh
curl http://IP_DU_MODULE/description.xml
curl -X POST http://IP_DU_MODULE/api -d '{"devicetype":"alexa"}'
curl http://IP_DU_MODULE/api/UTILISATEUR_RETOURNE/lights
```

Remplacer `UTILISATEUR_RETOURNE` par le champ `username` de la réponse POST,
puis reprendre un identifiant renvoyé par `lights` :

```sh
curl -X PUT http://IP_DU_MODULE/api/UTILISATEUR_RETOURNE/lights/IDENTIFIANT/state \
  -H 'Content-Type: application/json' -d '{"on":true}'
curl http://IP_DU_MODULE/api/UTILISATEUR_RETOURNE/lights/IDENTIFIANT
curl -X PUT http://IP_DU_MODULE/api/UTILISATEUR_RETOURNE/lights/IDENTIFIANT/state \
  -H 'Content-Type: application/json' -d '{"on":false}'
```

L'API accepte les lectures de configuration globale, des lumières et de leur état,
et la création d'un utilisateur. Les collections non prises en charge sont vides.
Les écritures acceptent uniquement un booléen `on` : variation et couleur ne sont
pas annoncées et ne sont pas appliquées. Un identifiant inconnu ou appartenant à
un autre module renvoie une erreur sans actionner les relais.

## Belkin Wemo : une prise par module

Pour utiliser l'émulation Wemo :

```text
backlog startDriver SSDP; startDriver Wemo
```

Ajouter `startDriver Wemo` au script de démarrage si ce mode est choisi.
Wemo expose une seule prise : le premier canal relais/Toggle dans l'ordre des
canaux. La lecture et la commande utilisent le même canal. Pour les modules LED,
le comportement Wemo existant de commande générale `POWER` est conservé.

Les deux pilotes peuvent fonctionner simultanément et répondent tous les deux aux
recherches SSDP génériques. Alexa peut alors créer des doublons pour un même relais ;
choisir un seul mode lors de l'appairage est généralement plus pratique.

Le descripteur est disponible sur `/setup.xml`, le service sur `/eventservice.xml`,
et les actions SOAP `GetBinaryState` / `SetBinaryState` sur
`/upnp/control/basicevent1`. Les valeurs d'écriture valides sont 0 et 1.

## Compilation et validation

Les pilotes `Hue`, `Wemo` et `SSDP` sont déjà activés dans la configuration standard
BK7231N ; leur démarrage reste explicite. Voir [BUILDING.md](../BUILDING.md) pour
les SDK et dépendances, puis compiler la cible correspondant au module :

```sh
make OpenBK7231N
```

Les tests hôte compilent les corps complets des pilotes Hue/Wemo avec le vrai
parseur jsmn et la structure HTTP du firmware, en remplaçant les appels matériels
et réseau par des stubs. Ils ne remplacent pas la compilation BK7231N ni un essai
avec un Echo physique :

```sh
python3 tests/alexa/test_alexa.py
```

Origine de l'adaptation : travail Hue de Switch-Gang (énumération des relais et
identifiants dérivés de la MAC). Les réponses SSDP sont regroupées en datagrammes
complets ; l'analyse JSON, la validation des canaux, la configuration Hue et les
réponses Wemo ont été adaptées au code actuel.
