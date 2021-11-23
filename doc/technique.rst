
.. include:: links.rst

=======================
Documentation technique
=======================

Divers
======

Voir :

* le `dépôt sur github`_
* le `sketch de l'arduino sur github`_


Photo de l'intérieur du thermostat
==================================

.. image:: ThermostatInterieur.png

Liste des éléments
==================

Les N° ci-dessous des éléments correspondent aux N° de la photo ci-dessus.

1. :index:`Arduino` NANO
2. Module :index:`RTC` (:index:`I2C`)
3. Connecteur d'arrivée de l'alimentation (**+** vers Vin de l'arduino)
4. connectique vers bouton **-**, bouton poussoir **13**
5. connectique vers bouton **+**, bouton poussoir **14**
6. connectique de gestion du commutateur 3 positions et des LEDs qui lui sont associées
7. connectique vers le relai SSR
8. résistance des ponts diviseurs associés au commutateur 6 positions (**12**)
9. ensemble des potentiomètres des ponts diviseurs associés au commutateur 6 positions (**12**)
10. connectique I2C vers l'afficheur OLED 0.96\"
11. connectique vers la sonde de température DS18B20
12. commutateur 6 positions de choix des modes de fonctionnement
13. bouton poussoir **-**
14. bouton poussoir **+**
15. interrupteur Marche/Arrêt
16. afficheur OLED 0.96\"
17. commutateur 3 positions
18. LED jaune (non visible sur cette photo)
19. LED bleue (à peine visible sur cette photo)
20. PCB de récupération avec le connecteur RJ45 pour les m-à-j de l'arduino via ISP/ICSP

Le rôle de l'arduino NANO
=========================


le module RTC
=============


le relai SSR et la connectique
==============================


La sonde DS18B20 et sa connectique
==================================


Les modes de fonctionnement et les ponts diviseurs
==================================================


Mise-à-jour du programme de l'arduino via ISP/ICSP
==================================================


Détails à propos du sketch de l'arduino
=======================================

Organisation du sketch

les interruptions

les hystérésis : de temps et de température




:index:`thermostat`