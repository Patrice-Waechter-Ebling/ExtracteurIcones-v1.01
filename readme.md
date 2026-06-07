#### WIN32 APPLICATION : ExtracteurIcones Project Overview

     Un point d'entrée classique pour une application Windows, qui crée la 
     fenêtre principale, affiche la fenêtre et lance la boucle de messages 
     il est important de noter que tout le travail de l'application 
     (création de la zone d'affichage, extraction des icônes, etc.) est 
     déclenché à partir de la procédure de fenêtre (WndProc) en réponse à 
     des messages spécifiques 
     (ex: WM_CREATE pour l'initialisation, WM_COMMAND pour les actions de menu, etc.)

#### Les fonctions spécifiques
    - AjustementCapaciteTableauInfoIcone 
      s'assure que le tableau global de stockage des infos d'icônes a la capacité nécessaire pour stocker les 
      infos d'une nouvelle icône (en doublant la capacité à chaque fois que c'est nécessaire)
    - IsExeOrDll 
      vérifie si le fichier a une extension .exe ou .dll (sans tenir compte de la casse) pour décider s'il 
      faut tenter d'en extraire les icônes ou pas
    - AjouterIconeDanZoneAffichage
      ajoute une icône à la zone d'affichage (ListView) et stocke les infos associées (chemin + index dans le fichier) 
      dans un tableau global pour pouvoir les réutiliser plus tard 
      (ex: lors d'un clic sur l'item pour afficher les détails de l'icône)
    - ExtraireIconesDesFichiers
      ne prend en compte que les fichiers .exe et .dll, et ignore les erreurs d'extraction d'icônes 
      (ex: pas de ressources d'icônes dans le fichier) et ajoute les icônes extraites à la zone d'affichage
    - AnalyserDossierRecursivement
      ignore les erreurs d'accès aux dossiers (ex: pas les droits nécessaires pour accéder à un dossier), et continue
      l'analyse récursive des autres dossiers ignore aussi les fichiers qui ne sont pas des .exe ou .dll et ajoute 
      les icônes extraites à la zone d'affichage.
      + <Attention>
      à noter que l'analyse récursive peut prendre beaucoup de temps si le dossier contient beaucoup de sous-dossiers
      et de fichiers, et que l'interface ne sera pas responsive pendant ce temps (pas de multithreading dans cette 
      application monolithique), mais c'est voulu pour garder l'exemple simple et se concentrer sur l'extraction 
      d'icônes et la gestion de l'interface dans une application plus complète, j'envisage d'ajouter une barre de
      progression et de faire l'analyse dans un thread  séparé pour garder l'interface responsive
