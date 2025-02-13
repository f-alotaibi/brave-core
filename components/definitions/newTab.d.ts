// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

declare namespace NewTab {
  // Custom background with solid color or gradation
  export type ColorBackground = {
    type: 'color'
    wallpaperColor: string
    random?: boolean
  }

  // Backgrounds based on image. Custom image background or Brave background.
  export type ImageBackground = {
    type: 'image'
    wallpaperImageUrl: string
    random?: boolean
  }

  export type BraveBackground = Omit<ImageBackground, 'type'> & {
    type: 'brave'
    author: string
    link?: string
    originalUrl?: string
    license?: string

    // Picked by our rotating algorithm. When it's false, it means that the user
    // has picked this background.
    random?: boolean
  }

  export type BackgroundWallpaper = ColorBackground | ImageBackground | BraveBackground

  export type BrandedWallpaperLogo = {
    image: string
    companyName: string
    alt: string
    destinationUrl: string
  }

  export type BrandedWallpaper = {
    wallpaperImageUrl: string
    isSponsored: boolean
    creativeInstanceId: string
    wallpaperId: string
    logo: BrandedWallpaperLogo
  }

  export interface Wallpaper {
    backgroundWallpaper: BackgroundWallpaper
    brandedWallpaper?: BrandedWallpaper
  }

  export interface ApplicationState {
    newTabData: State | undefined
    gridSitesData: GridSitesState | undefined
  }

  export interface Site {
    id: string
    url: string
    title: string
    favicon: string
    letter: string
    pinnedIndex: number | undefined
    defaultSRTopSite: boolean | undefined
  }

  export interface Stats {
    adsBlockedStat: number
    httpsUpgradesStat: number
    javascriptBlockedStat: number
    bandwidthSavedStat: number
    fingerprintingBlockedStat: number
  }

  export interface Bookmark {
    dateAdded: number
    id: string
    index: number
    parentId: string
    title: string
    url: string
  }

  export type StackWidget =  'braveVPN' | ''

  export interface GridSitesState {
    removedSites: Site[]
    gridSites: Site[]
    shouldShowSiteRemovedNotification: boolean
  }

  export interface PageState {
    showEmptyPage: boolean
  }

  export interface PersistentState {
    showEmptyPage: boolean
    currentStackWidget: StackWidget
    removedStackWidgets: StackWidget[]
    widgetStackOrder: StackWidget[]
  }

  export type Preferences = {
    showBackgroundImage: boolean
    brandedWallpaperOptIn: boolean
    showStats: boolean
    showToday: boolean
    showClock: boolean
    clockFormat: string
    showTopSites: boolean
    showBraveVPN: boolean
    showSearchBox: boolean
    lastUsedNtpSearchEngine: string
    promptEnableSearchSuggestions: boolean
    searchSuggestionsEnabled: boolean
    hideAllWidgets: boolean
    isBrandedWallpaperNotificationDismissed: boolean
  }

  export type EphemeralState = Preferences & {
    initialDataLoaded: boolean
    textDirection: string
    searchPromotionEnabled: boolean
    featureCustomBackgroundEnabled: boolean
    isIncognito: boolean
    torCircuitEstablished: boolean,
    torInitProgress: string,
    isTor: boolean
    gridLayoutSize?: 'small'
    showGridSiteRemovedNotification?: boolean
    showBackgroundImage: boolean
    customLinksEnabled: boolean
    customLinksNum: number
    showBitcoinDotCom: boolean
    stats: Stats,
    brandedWallpaper?: BrandedWallpaper
    backgroundWallpaper?: BackgroundWallpaper
    customImageBackgrounds: ImageBackground[]
  }

  interface StorybookStateExtras {
    forceSettingsTab?: string // SettingsTabType
    readabilityThreshold?: number
  }

  // In-memory state is a superset of PersistentState
  export type State = PersistentState & EphemeralState & StorybookStateExtras
}
