export type AccountType = "msa" | "offline";

/** Mirrors `AccountState` in launcher/minecraft/auth/AccountData.h. */
export type AccountState =
  | "unchecked"
  | "offline"
  | "working"
  | "online"
  | "disabled"
  | "errored"
  | "expired"
  | "gone";

/** Serialized by `AccountApi::serializeAccount`. */
export interface Account {
  /** Launcher-internal account id (stable across refreshes). */
  id: string;
  profileId: string;
  profileName: string;
  type: AccountType;
  state: AccountState;
  isDefault: boolean;
  ownsMinecraft: boolean;
  hasProfile: boolean;
  /** Player face rendered by the backend, `materialmc://app/_face/<id>`. */
  faceUrl: string;
  lastError: string;
}

export interface LoginFlowHandle {
  flowId: string;
}
