namespace alisp { inline const char* getInitCode() { return R"code(

(defun cadr (cons-cell)
  (nth 1 cons-cell))

(defun cdar (cons-cell)
  (cdr (car cons-cell)))

(defun car-safe (object)
  (let ((x object))
    (if (consp x)
        (car x)
      nil)))

(defun cdr-safe (object)
  (let ((x object))
    (if (consp x)
        (cdr x)
      nil)))

(defmacro pop (listname)
  (list 'prog1 (list 'car listname)
        (list 'setq listname 
              (list 'cdr listname))))

(defmacro push (element listname)
  (list 'setq listname
        (list 'cons element listname)))

(defmacro setq (sym var)
  (list 'set (list 'quote sym) var))

)code"; }}
