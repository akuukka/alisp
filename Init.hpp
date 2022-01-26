namespace alisp { inline const char* getInitCode() { return R"code(

(defmacro setq (sym var)
  (list 'set (list 'quote sym) var))

(defmacro pop (listname)
  (list 'prog1 (list 'car listname)
        (list 'setq listname 
              (list 'cdr listname))))

(defmacro push (element listname)
  (list 'setq listname
        (list 'cons element listname)))


)code"; }}
