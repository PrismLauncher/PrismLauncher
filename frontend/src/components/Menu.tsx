import { useEffect, useRef, useState, type ReactNode } from "react";

export interface MenuItem {
  label: string;
  onSelect(): void;
  danger?: boolean;
  disabled?: boolean;
  separatorBefore?: boolean;
}

/** A "⋮" button that opens a small popup menu. */
export function MenuButton({ items, label = "More actions", children }: { items: MenuItem[]; label?: string; children?: ReactNode }) {
  const [open, setOpen] = useState(false);
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const close = (e: MouseEvent) => {
      if (!ref.current?.contains(e.target as Node)) setOpen(false);
    };
    const onKey = (e: KeyboardEvent) => e.key === "Escape" && setOpen(false);
    window.addEventListener("mousedown", close);
    window.addEventListener("keydown", onKey);
    return () => {
      window.removeEventListener("mousedown", close);
      window.removeEventListener("keydown", onKey);
    };
  }, [open]);

  return (
    <div ref={ref} style={{ position: "relative" }}>
      <button className="btn icon-only ghost" aria-label={label} aria-haspopup="menu" onClick={() => setOpen((o) => !o)}>
        {children ?? "⋮"}
      </button>
      {open && (
        <div className="menu" role="menu" style={{ right: 0, top: "calc(100% + 4px)" }}>
          {items.map((item) => (
            <div key={item.label} style={{ display: "contents" }}>
              {item.separatorBefore && <hr />}
              <button
                role="menuitem"
                className={item.danger ? "danger" : undefined}
                disabled={item.disabled}
                onClick={() => {
                  setOpen(false);
                  item.onSelect();
                }}
              >
                {item.label}
              </button>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
