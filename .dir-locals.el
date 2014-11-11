;; <http://www.gnu.org/software/emacs/manual/html_node/emacs/Directory-Variables.html>
(( nil . ((eval . (progn 
                    (require 'projectile)
                    (puthash (projectile-project-root) "cd build; make -j 4"  projectile-compilation-cmd-map)
                    (puthash (projectile-project-root) "cd build; ctest -j 4" projectile-test-cmd-map)))))
 ( c-mode . ((c-basic-offset . 4)
             (tab-width . 8)
             (indent-tabs-mode . nil))))

