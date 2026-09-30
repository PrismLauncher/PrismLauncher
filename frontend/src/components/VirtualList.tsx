import { forwardRef, useCallback, useEffect, useImperativeHandle, useRef, useState, type ReactNode } from "react";

export interface VirtualListHandle {
  scrollToBottom(): void;
  isAtBottom(): boolean;
}

interface VirtualListProps<T> {
  items: readonly T[];
  rowHeight: number;
  renderRow(item: T, index: number): ReactNode;
  getKey(item: T, index: number): string | number;
  className?: string;
  style?: React.CSSProperties;
  overscan?: number;
  /** Called when the user scrolls; `atBottom` is true within one row of the end. */
  onScroll?(atBottom: boolean): void;
}

/**
 * Fixed-row-height windowed list: only rows in (and near) the viewport are mounted,
 * so 100k console lines or thousands of versions stay cheap to render.
 */
function VirtualListInner<T>(
  { items, rowHeight, renderRow, getKey, className, style, overscan = 8, onScroll }: VirtualListProps<T>,
  ref: React.ForwardedRef<VirtualListHandle>,
) {
  const container = useRef<HTMLDivElement>(null);
  const [scrollTop, setScrollTop] = useState(0);
  const [height, setHeight] = useState(0);

  useEffect(() => {
    const el = container.current;
    if (!el) return;
    const observer = new ResizeObserver(() => setHeight(el.clientHeight));
    observer.observe(el);
    setHeight(el.clientHeight);
    return () => observer.disconnect();
  }, []);

  const atBottom = useCallback(() => {
    const el = container.current;
    if (!el) return true;
    return el.scrollHeight - el.scrollTop - el.clientHeight < rowHeight * 1.5;
  }, [rowHeight]);

  useImperativeHandle(
    ref,
    () => ({
      scrollToBottom() {
        const el = container.current;
        if (el) el.scrollTop = el.scrollHeight;
      },
      isAtBottom: atBottom,
    }),
    [atBottom],
  );

  const first = Math.max(0, Math.floor(scrollTop / rowHeight) - overscan);
  const last = Math.min(items.length, Math.ceil((scrollTop + height) / rowHeight) + overscan);
  const rows: ReactNode[] = [];
  for (let i = first; i < last; i++) {
    const item = items[i] as T;
    rows.push(
      <div key={getKey(item, i)} style={{ position: "absolute", top: i * rowHeight, left: 0, right: 0, height: rowHeight }}>
        {renderRow(item, i)}
      </div>,
    );
  }

  return (
    <div
      ref={container}
      className={className}
      style={{ overflowY: "auto", position: "relative", ...style }}
      onScroll={(e) => {
        setScrollTop(e.currentTarget.scrollTop);
        onScroll?.(atBottom());
      }}
    >
      <div style={{ height: items.length * rowHeight, position: "relative" }}>{rows}</div>
    </div>
  );
}

export const VirtualList = forwardRef(VirtualListInner) as <T>(
  props: VirtualListProps<T> & { ref?: React.Ref<VirtualListHandle> },
) => ReturnType<typeof VirtualListInner>;
